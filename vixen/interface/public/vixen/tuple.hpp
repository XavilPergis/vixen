#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/traits.hpp"
#include "vixen/typeops.hpp"
#include "vixen/types.hpp"

namespace vixen {

struct tuple_concat_tag {};

template <usize Index, typename... Ts>
struct pack_select;

template <typename T, typename... Ts>
struct pack_select<0, T, Ts...> {
    using type = T;
};

template <usize Index, typename T, typename... Ts>
struct pack_select<Index, T, Ts...> : pack_select<Index - 1, Ts...> {};

template <usize L, typename T>
struct ValueHolder {
    ValueHolder() = default;
    ValueHolder(ValueHolder &&) = default;
    ValueHolder(ValueHolder const &) = default;
    ValueHolder &operator=(ValueHolder &&) = default;
    ValueHolder &operator=(ValueHolder const &) = default;

    template <typename U>
    ValueHolder(U &&val) : value(std::forward<U>(val)) {}

    ValueHolder(copy_tag_t, Allocator *alloc, ValueHolder const &other)
        : value(copy_construct_maybe_allocator_aware(alloc, other.value)) {}

    T value;
};

template <usize Index, typename... Ts>
struct TupleImpl;

template <usize Index, typename A, typename B, typename... Ts>
struct TupleImpl<Index, A, B, Ts...>
    : ValueHolder<Index, A>
    , TupleImpl<Index + 1, B, Ts...> {
    using Base = TupleImpl<Index + 1, B, Ts...>;
    using Holder = ValueHolder<Index, A>;

    TupleImpl() = default;
    TupleImpl(TupleImpl &&other) = default;
    TupleImpl(TupleImpl const &other) = default;
    TupleImpl &operator=(TupleImpl &&other) = default;
    TupleImpl &operator=(TupleImpl const &other) = default;

    template <typename U, typename... Us>
    TupleImpl(U &&head, Us &&...values)
        : Holder(std::forward<U>(head)), Base(std::forward<Us>(values)...) {}

    TupleImpl(copy_tag_t, Allocator *alloc, TupleImpl const &other)
        : Holder(copy_tag, alloc, static_cast<Holder const &>(other))
        , Base(copy_tag, alloc, static_cast<Base const &>(other)) {}
};

template <usize Index, typename T>
struct TupleImpl<Index, T> : ValueHolder<Index, T> {
    using Holder = ValueHolder<Index, T>;

    TupleImpl() = default;
    TupleImpl(TupleImpl &&other) = default;
    TupleImpl(TupleImpl const &other) = default;
    TupleImpl &operator=(TupleImpl &&other) = default;
    TupleImpl &operator=(TupleImpl const &other) = default;

    template <typename U>
    TupleImpl(U &&head) : Holder(std::forward<U>(head)) {}

    TupleImpl(copy_tag_t, Allocator *alloc, TupleImpl const &other)
        : Holder(copy_tag, alloc, static_cast<Holder const &>(other)) {}
};

// --------------------------------------------------------------------------------

template <typename... Ts>
struct Tuple : TupleImpl<0, Ts...> {
    template <typename... Us, require<std::conjunction<std::is_constructible<Ts, Us>...>> = true>
    Tuple(Us &&...values) : TupleImpl<0, Ts...>(std::forward<Us>(values)...) {}

    Tuple() = default;
    Tuple(Tuple &&) = default;
    Tuple(Tuple const &) = default;
    Tuple &operator=(Tuple &&) = default;
    Tuple &operator=(Tuple const &) = default;

    Tuple(copy_tag_t, Allocator *alloc, Tuple const &other)
        : TupleImpl<0, Ts...>(copy_tag, alloc, static_cast<TupleImpl<0, Ts...> const &>(other)) {}

    template <usize I>
    typename pack_select<I, Ts...>::type &get() {
        return ValueHolder<I, typename pack_select<I, Ts...>::type>::value;
    }

    template <usize I>
    typename pack_select<I, Ts...>::type const &get() const {
        return ValueHolder<I, typename pack_select<I, Ts...>::type>::value;
    }

    // template <typename F>
    // void each(F &&func) {
    //     TupleImpl<0, Ts>::each(std::forward<F>(func));
    // }

    // template <typename T, typename F>
    // T fold(T &&acc, F &&func) {
    //     return TupleImpl<0, Ts>::fold(std::forward<T>(acc), func);
    // }

    // Tuple<A, B, C, D> -> remove<2>() -> Tuple<C, Tuple<A, B, D>>
    // template <usize I>
    // Tuple<select<I, Ts>, remove<I, Ts>> remove() {}

    // template <usize I, typename T>
    // typename Insert<I, Ts...>::Type insert(const &value);
};

template <usize I, typename... Ts>
typename pack_select<I, Ts...>::type &get(Tuple<Ts...> &tup) {
    return tup.template get<I>();
}

template <usize I, typename... Ts>
typename pack_select<I, Ts...>::type &get(Tuple<Ts...> const &tup) {
    return tup.template get<I>();
}

template <typename... Ts>
constexpr Tuple<Ts &&...> forward_as_tuple(Ts &&...args) noexcept {
    return Tuple<Ts &&...>(std::forward<Ts>(args)...);
}

template <typename... Ts>
struct type_sequence {};

template <typename... Ts, typename... Us>
type_sequence<Ts..., Us...> concat_type_seq_impl(type_sequence<Ts...>, type_sequence<Us...>) {
    return type_sequence<Ts..., Us...>{};
}

template <typename A, typename B>
using concat_type_sequence = decltype(concat_type_seq_impl(A{}, B{}));

template <usize... Ixs>
struct index_sequence {};

template <usize Len, usize... Ixs>
auto make_index_sequence_impl() {
    if constexpr (Len == 0) {
        return index_sequence<Ixs...>{};
    } else {
        return make_index_sequence_impl<Len - 1, Len - 1, Ixs...>();
    }
}

template <usize Len>
using make_index_sequence = decltype(make_index_sequence_impl<Len>());

template <typename... Ts>
Tuple<Ts...> make_tuple(Ts &&...values) {
    return Tuple<Ts...>{std::forward<Ts>(values)...};
}

// clang-format off
template <typename... Ts, typename... Us, usize... TIs, usize... UIs>
Tuple<Ts..., Us...> concat_tuples_seq(Tuple<Ts...> first, Tuple<Us...> last, index_sequence<TIs...>, index_sequence<UIs...>) {
    return Tuple<Ts..., Us...>{
        static_cast<Ts&&>(get<TIs>(first))...,
        static_cast<Us&&>(get<UIs>(last))...
    };
}
// clang-format on

template <typename... Ts, typename... Us>
Tuple<Ts..., Us...> concat_tuples(Tuple<Ts...> first, Tuple<Us...> last) {
    return concat_tuples_seq(first,
        last,
        make_index_sequence<sizeof...(Ts)>{},
        make_index_sequence<sizeof...(Us)>{});
}

} // namespace vixen
