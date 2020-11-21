#pragma once

#include "vixen/option.hpp"
#include "vixen/types.hpp"

#define DECLVAL(Ty) std::declval<Ty>()

namespace vixen::iter {

template <typename Iter>
using iter_output = typename Iter::output;

#pragma region "Placeholders"

namespace placeholder {

template <typename F>
struct map_iterator {
    F mapper;
};

template <typename F>
struct filter_iterator {
    F predicate;
};

template <typename Coll>
struct back_inserter_iterator {
    Coll *collection;
};

} // namespace placeholder

#pragma endregion
#pragma region "Iterators"

template <typename Iter, typename F>
struct map_iterator {
    using output = decltype(DECLVAL(F)(DECLVAL(iter_output<Iter> &&)));

    Iter iter;
    F mapper;

    option<output> next();
};

template <typename Iter, typename F>
struct filter_iterator {
    using output = iter_output<Iter> &&;

    Iter iter;
    F predicate;

    option<output> next();
};

template <typename T>
struct slice_iterator {
    using output = T &;

    slice<T> inner;
    usize cur;

    option<output> next();
};

#pragma endregion
#pragma region "Iterator Functions"

template <typename T>
inline slice_iterator<T> slice_iter(slice<T> slice);

template <typename F>
inline placeholder::filter_iterator<F> filter(F &&predicate);

template <typename F>
inline placeholder::map_iterator<F> map(F &&mapper);

template <typename Coll>
inline placeholder::back_inserter_iterator<Coll> back_inserter(Coll *coll);

#pragma endregion
#pragma region "`operator|` Impls"

template <typename I, typename F>
inline filter_iterator<I, F> operator|(I &&iter, placeholder::filter_iterator<F> &&filter);

template <typename I, typename F>
inline map_iterator<I, F> operator|(I &&iter, placeholder::map_iterator<F> &&map);

template <typename I, typename Coll>
inline void operator|(I &&iter, placeholder::back_inserter_iterator<Coll> &&back_inserter);

#pragma endregion

} // namespace vixen::iter

#include "iterator.inl"
