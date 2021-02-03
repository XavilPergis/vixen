#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/traits.hpp"
#include "vixen/typeops.hpp"
#include "vixen/types.hpp"

namespace vixen {
template <usize L, typename T>
struct value_holder {
    value_holder(T &&value) : value(mv(value)) {}
    value_holder(T const &value) : value(value) {}
    value_holder() = default;

    value_holder(value_holder &&other) = default;
    value_holder(value_holder const &other) = default;
    value_holder(allocator *alloc, value_holder const &other)
        : value(copy_construct_maybe_allocator_aware(alloc, other.value)) {}

    T &get() {
        return value;
    }

    T const &get() const {
        return value;
    }

    T value;
};

template <usize L, typename Ts>
struct tuple_impl_base
    : public tuple_impl_base<L + 1, tail<Ts>>
    , public value_holder<L, head<Ts>> {
    using base = tuple_impl_base<L + 1, tail<Ts>>;
    using holder = value_holder<L, head<Ts>>;

    template <typename U, typename... Us>
    tuple_impl_base(U &&value, Us &&...values)
        : base(std::forward<Us>(values)...), holder(std::forward<U>(value)) {}
    tuple_impl_base() = default;

    tuple_impl_base(tuple_impl_base &&other) = default;
    tuple_impl_base(tuple_impl_base const &other) = default;
    tuple_impl_base(allocator *alloc, tuple_impl_base const &other)
        : base(alloc, static_cast<base const &>(other))
        , holder(alloc, static_cast<holder const &>(other)) {}

    template <typename F>
    void each(F &&func) {
        func(holder::get());
        base::each(func);
    }

    template <typename T, typename F>
    T fold(T &&acc, F &func) {
        return base::fold(func(std::forward<T>(acc), holder::get()));
    }
};

template <usize L>
struct tuple_impl_base<L, nil> {
    tuple_impl_base() {}

    tuple_impl_base(tuple_impl_base &&other) = default;
    tuple_impl_base(tuple_impl_base const &other) = default;
    tuple_impl_base(allocator *alloc, tuple_impl_base const &other) {}

    template <typename F>
    void each(F &&func) {}

    template <typename T, typename F>
    T fold(T &&acc, F &func) {
        return std::forward<T>(acc);
    }
};

// --------------------------------------------------------------------------------

template <typename Ts>
struct tuple_impl : tuple_impl_base<0, Ts> {
    template <typename... Us>
    tuple_impl(Us &&...values) : tuple_impl_base<0, Ts>(std::forward<Us>(values)...) {}
    tuple_impl() : tuple_impl_base<0, Ts>() {}

    tuple_impl(tuple_impl &&other) = default;
    tuple_impl(tuple_impl const &other) = default;
    tuple_impl(allocator *alloc, tuple_impl const &other)
        : tuple_impl_base<0, Ts>(alloc, static_cast<tuple_impl_base<0, Ts> const &>(other)) {}

    template <usize I>
    select<I, Ts> &get() {
        return value_holder<I, select<I, Ts>>::get();
    }

    template <usize I>
    select<I, Ts> const &get() const {
        return value_holder<I, select<I, Ts>>::get();
    }

    template <typename F>
    void each(F &&func) {
        tuple_impl_base<0, Ts>::each(std::forward<F>(func));
    }

    template <typename T, typename F>
    T fold(T &&acc, F &&func) {
        return tuple_impl_base<0, Ts>::fold(std::forward<T>(acc), func);
    }

    // tuple<A, B, C, D> -> remove<2>() -> tuple<C, tuple<A, B, D>>
    // template <usize I>
    // tuple<select<I, Ts>, remove<I, Ts>> remove() {}

    // template <usize I, typename T>
    // typename Insert<I, Ts...>::Type insert(const &value);
};

template <typename Ts>
using cons_tuple = tuple_impl<Ts>;

template <typename... Ts>
using tuple = tuple_impl<unpack<Ts...>>;

// template <typename... Ts>
// void destruct(tuple<Ts...> &tup)
// {}

} // namespace vixen
