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
struct value_holder {
    value_holder() = default;
    value_holder(value_holder &&) = default;
    value_holder(value_holder const &) = default;
    value_holder &operator=(value_holder &&) = default;
    value_holder &operator=(value_holder const &) = default;

    template <typename U>
    value_holder(U &&val) : value(std::forward<U>(val)) {}

    value_holder(copy_tag_t, allocator *alloc, value_holder const &other)
        : value(copy_construct_maybe_allocator_aware(alloc, other.value)) {}

    T value;
};

template <usize Index, typename... Ts>
struct tuple_impl;

template <usize Index, typename A, typename B, typename... Ts>
struct tuple_impl<Index, A, B, Ts...>
    : value_holder<Index, A>
    , tuple_impl<Index + 1, B, Ts...> {
    using base = tuple_impl<Index + 1, B, Ts...>;
    using holder = value_holder<Index, A>;

    tuple_impl() = default;
    tuple_impl(tuple_impl &&other) = default;
    tuple_impl(tuple_impl const &other) = default;
    tuple_impl &operator=(tuple_impl &&other) = default;
    tuple_impl &operator=(tuple_impl const &other) = default;

    template <typename U, typename... Us>
    tuple_impl(U &&head, Us &&...values)
        : holder(std::forward<U>(head)), base(std::forward<Us>(values)...) {}

    tuple_impl(copy_tag_t, allocator *alloc, tuple_impl const &other)
        : holder(copy_tag, alloc, static_cast<holder const &>(other))
        , base(copy_tag, alloc, static_cast<base const &>(other)) {}
};

template <usize Index, typename T>
struct tuple_impl<Index, T> : value_holder<Index, T> {
    using holder = value_holder<Index, T>;

    tuple_impl() = default;
    tuple_impl(tuple_impl &&other) = default;
    tuple_impl(tuple_impl const &other) = default;
    tuple_impl &operator=(tuple_impl &&other) = default;
    tuple_impl &operator=(tuple_impl const &other) = default;

    template <typename U>
    tuple_impl(U &&head) : holder(std::forward<U>(head)) {}

    tuple_impl(copy_tag_t, allocator *alloc, tuple_impl const &other)
        : holder(copy_tag, alloc, static_cast<holder const &>(other)) {}
};

// --------------------------------------------------------------------------------

template <typename... Ts>
struct tuple : tuple_impl<0, Ts...> {
    template <typename... Us, require<std::conjunction<std::is_constructible<Ts, Us>...>> = true>
    tuple(Us &&...values) : tuple_impl<0, Ts...>(std::forward<Us>(values)...) {}

    tuple() = default;
    tuple(tuple &&) = default;
    tuple(tuple const &) = default;
    tuple &operator=(tuple &&) = default;
    tuple &operator=(tuple const &) = default;

    tuple(copy_tag_t, allocator *alloc, tuple const &other)
        : tuple_impl<0, Ts...>(copy_tag, alloc, static_cast<tuple_impl<0, Ts...> const &>(other)) {}

    template <usize I>
    typename pack_select<I, Ts...>::type &get() {
        return value_holder<I, typename pack_select<I, Ts...>::type>::value;
    }

    template <usize I>
    typename pack_select<I, Ts...>::type const &get() const {
        return value_holder<I, typename pack_select<I, Ts...>::type>::value;
    }

    // template <typename F>
    // void each(F &&func) {
    //     tuple_impl<0, Ts>::each(std::forward<F>(func));
    // }

    // template <typename T, typename F>
    // T fold(T &&acc, F &&func) {
    //     return tuple_impl<0, Ts>::fold(std::forward<T>(acc), func);
    // }

    // tuple<A, B, C, D> -> remove<2>() -> tuple<C, tuple<A, B, D>>
    // template <usize I>
    // tuple<select<I, Ts>, remove<I, Ts>> remove() {}

    // template <usize I, typename T>
    // typename Insert<I, Ts...>::Type insert(const &value);
};

template <usize I, typename... Ts>
typename pack_select<I, Ts...>::type &get(tuple<Ts...> &tup) {
    return tup.template get<I>();
}

template <usize I, typename... Ts>
typename pack_select<I, Ts...>::type &get(tuple<Ts...> const &tup) {
    return tup.template get<I>();
}

template <typename... Ts>
constexpr tuple<Ts &&...> forward_as_tuple(Ts &&...args) noexcept {
    return tuple<Ts &&...>(std::forward<Ts>(args)...);
}

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
tuple<Ts...> make_tuple(Ts &&...values) {
    return tuple<Ts...>{std::forward<Ts>(values)...};
}

// clang-format off
template <typename... Ts, typename... Us, usize... TIs, usize... UIs>
tuple<Ts..., Us...> concat_tuples_seq(tuple<Ts...> first, tuple<Us...> last, index_sequence<TIs...>, index_sequence<UIs...>) {
    return tuple<Ts..., Us...>{
        static_cast<Ts&&>(get<TIs>(first))...,
        static_cast<Us&&>(get<UIs>(last))...
    };
}
// clang-format on

template <typename... Ts, typename... Us>
tuple<Ts..., Us...> concat_tuples(tuple<Ts...> first, tuple<Us...> last) {
    return concat_tuples_seq(first,
        last,
        make_index_sequence<sizeof...(Ts)>{},
        make_index_sequence<sizeof...(Us)>{});
}

} // namespace vixen
