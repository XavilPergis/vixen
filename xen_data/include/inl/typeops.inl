#pragma once

#include "xen/tuple.hpp"

namespace xen {
template <typename H, typename... Ts>
struct unpack_impl<H, Ts...> {
    using type = cons<H, unpack<Ts...>>;
};

template <>
struct unpack_impl<> {
    using type = nil;
};

// --------------------------------------------------------------------------------

template <typename Ts, typename T>
struct append_impl {
    using type = cons<append<T, tail<Ts>>, head<Ts>>;
};

template <typename T>
struct append_impl<nil, T> {
    using type = cons<T, nil>;
};

// --------------------------------------------------------------------------------

template <typename As, typename Bs>
struct concat_impl {
    using type = concat<append<As, head<Bs>>, tail<Bs>>;
};

template <typename As>
struct concat_impl<As, nil> {
    using type = As;
};

// --------------------------------------------------------------------------------

template <usize K, typename Ts>
struct drop_impl {
    using type = drop<K - 1, tail<Ts>>;
};

template <typename Ts>
struct drop_impl<0, Ts> {
    using type = Ts;
};

// --------------------------------------------------------------------------------

template <usize I, typename Ts>
struct select_impl {
    using type = select<I - 1, tail<Ts>>;
};

template <typename Ts>
struct select_impl<0, Ts> {
    using type = head<Ts>;
};

// --------------------------------------------------------------------------------

template <template <typename X> typename F, typename Ts>
struct map_impl {
    using mapped = typename F<head<Ts>>::type;
    using rest = map<F, tail<Ts>>;
    using type = cons<mapped, rest>;
};

template <template <typename X> typename F>
struct map_impl<F, nil> {
    using type = nil;
};

// --------------------------------------------------------------------------------

template <typename T, typename F>
struct choice_impl<true, T, F> {
    using type = T;
};

template <typename T, typename F>
struct choice_impl<false, T, F> {
    using type = F;
};

// --------------------------------------------------------------------------------

template <template <typename X> typename P, typename Ts>
struct filter_impl {
    constexpr static bool predicate = P<head<Ts>>::value;
    using rest = filter<P, tail<Ts>>;
    using type = choice<predicate, cons<head<Ts>, rest>, rest>;
};

template <template <typename X> typename P>
struct filter_impl<P, nil> {
    using type = nil;
};

// --------------------------------------------------------------------------------

template <usize K, typename T>
struct repeat_impl {
    using type = cons<T, repeat<K - 1, T>>;
};

template <typename T>
struct repeat_impl<0, T> {
    using type = nil;
};

// --------------------------------------------------------------------------------
} // namespace xen
