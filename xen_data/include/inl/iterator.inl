#pragma once

#include "xen/iterator.hpp"

namespace xen::iter {

#pragma region "next() impls"

template <typename I, typename F>
option<iter_output<map_iterator<I, F>>> map_iterator<I, F>::next() {
    option<iter_output<I>> item = iter.next();
    if (item.is_none()) {
        return {};
    }
    return mapper(*item);
}

template <typename I, typename F>
option<iter_output<filter_iterator<I, F>>> filter_iterator<I, F>::next() {
    option<iter_output<I>> item;

    do {
        item = iter.next();
        if (item.is_none()) {
            return {};
        }
    } while (!predicate(*item));
    return item;
}

template <typename T>
option<iter_output<slice_iterator<T>>> slice_iterator<T>::next() {
    if (cur == inner.len) {
        return {};
    }

    return inner[cur++];
}

#pragma endregion
#pragma region "Iterator Functions"

template <typename T>
inline slice_iterator<T> slice_iter(slice<T> inner) {
    return slice_iterator<T>{inner, 0};
}

template <typename F>
inline placeholder::map_iterator<F> filter(F &&mapper) {
    return placeholder::map_iterator<F>{mapper};
}

template <typename F>
inline placeholder::filter_iterator<F> filter(F &&predicate) {
    return placeholder::filter_iterator<F>{predicate};
}

template <typename Coll>
inline placeholder::back_inserter_iterator<Coll> back_inserter(Coll *coll) {
    return placeholder::back_inserter_iterator<Coll>{coll};
}

#pragma endregion
#pragma region "`operator|` Impls"

template <typename I, typename F>
inline map_iterator<I, F> operator|(I &&iter, placeholder::map_iterator<F> &&map) {
    return map_iterator<I, F>{iter, map.mapper};
}

template <typename I, typename F>
inline filter_iterator<I, F> operator|(I &&iter, placeholder::filter_iterator<F> &&filter) {
    return filter_iterator<I, F>{iter, filter.predicate};
}

template <typename I, typename Coll>
inline void operator|(I &&iter, placeholder::back_inserter_iterator<Coll> &&back_inserter) {
    while (option<iter_output<I>> item = iter.next()) {
        back_inserter.collection->push(*item);
    }
}

#pragma endregion

} // namespace xen::iter
