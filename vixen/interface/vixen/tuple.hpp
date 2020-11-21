#pragma once

#include "vixen/traits.hpp"
#include "vixen/typeops.hpp"
#include "vixen/types.hpp"

namespace vixen {
template <usize L, typename T>
struct value_holder {
    value_holder(T const &value) : value(value) {}
    value_holder() : value(T{}) {}

    T &get() {
        return value;
    }

    T value;
};

template <usize L, typename Ts>
struct tuple_impl_base
    : public tuple_impl_base<L + 1, tail<Ts>>
    , public value_holder<L, head<Ts>> {
    template <typename U, typename... Us>
    tuple_impl_base(U const &value, Us const &...values)
        : tuple_impl_base<L + 1, tail<Ts>>(values...), value_holder<L, head<Ts>>(value) {}
    tuple_impl_base() : tuple_impl_base<L + 1, tail<Ts>>(), value_holder<L, head<Ts>>() {}

    template <typename F>
    void each(F &&func) {
        func(value_holder<L, head<Ts>>::get());
        tuple_impl_base<L + 1, tail<Ts>>::each(func);
    }
};

template <usize L>
struct tuple_impl_base<L, nil> {
    tuple_impl_base() {}

    template <typename F>
    void each(F &&func) {}
};

// --------------------------------------------------------------------------------

template <typename Ts>
struct tuple_impl : public tuple_impl_base<0, Ts> {
    template <typename... Us>
    tuple_impl(Us const &...values) : tuple_impl_base<0, Ts>(values...) {}
    tuple_impl() : tuple_impl_base<0, Ts>() {}

    template <usize I>
    select<I, Ts> &get() {
        using Base = value_holder<I, select<I, Ts>>;
        return Base::get();
    }

    template <typename F>
    void each(F &&func) {
        tuple_impl_base<0, Ts>::each(func);
    }

    // tuple<A, B, C, D> -> remove<2>() -> tuple<C, tuple<A, B, D>>
    // template <usize I>
    // tuple<At<I>, Remove<I>> remove();

    // template <usize I, typename T>
    // typename Insert<I, Ts...>::Type insert(const &value);
};

template <typename... Ts>
using tuple = tuple_impl<unpack<Ts...>>;

// template <typename... Ts>
// void destruct(tuple<Ts...> &tup)
// {}

} // namespace vixen
