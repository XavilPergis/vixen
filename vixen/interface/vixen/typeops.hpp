#pragma once

#include "vixen/types.hpp"

namespace vixen {
// --------------------------------------------------------------------------------

struct nil {};

template <typename T, typename L>
struct cons : public L {
    using head = T;
    using tail = L;
};

template <typename Ts>
using head = typename Ts::head;

template <typename Ts>
using tail = typename Ts::tail;

// --------------------------------------------------------------------------------

// Converts a parameter pack into a cons-list.
// I.e., `unpack<A, B, C>` = `cons<A, cons<B, cons<C, nil>>>`

template <typename... Ts>
struct unpack_impl;

template <typename... Ts>
using unpack = typename unpack_impl<Ts...>::type;

// --------------------------------------------------------------------------------

// len :: [a] -> Int
// len [_|ts] = 1 + len ts
// len [] = 0

template <typename Ts>
struct len {
    constexpr static usize size = 1 + len<tail<Ts>>::size;
};

template <>
struct len<nil> {
    constexpr static usize size = 0;
};

// --------------------------------------------------------------------------------

// append :: [a] -> a -> [a]
// append [t|ts] a = t : append ts a
// append [] a = a

template <typename Ts, typename T>
struct appen_impl;

template <typename Ts, typename T>
using append = typename appen_impl<Ts, T>::type;

// --------------------------------------------------------------------------------

// concat :: [a] -> [a] -> [a]
// concat as b:bs = concat (append as b) bs
// concat as [] = as

template <typename As, typename Bs>
struct concat_impl;

template <typename As, typename Bs>
using concat = typename concat_impl<As, Bs>::type;

// --------------------------------------------------------------------------------

// drop :: Int -> [a] -> [a]
// drop k _:ts = drop (k-1) ts
// drop 0 ts = ts

template <usize K, typename Ts>
struct drop_impl;

template <usize K, typename Ts>
using drop = typename drop_impl<K, Ts>::type;

// --------------------------------------------------------------------------------

// select :: Int -> [a] -> a
// select k _:ts = select (k-1) ts
// select 0 t:_ = t

template <usize I, typename Ts>
struct select_impl;

template <usize I, typename Ts>
using select = typename select_impl<I, Ts>::type;

// --------------------------------------------------------------------------------

// map :: (a -> b) -> [a] -> [b]
// map f t:ts = f t : map (f) ts
// map f [] = []

template <template <typename X> typename F, typename Ts>
struct map_impl;

template <template <typename X> typename F, typename Ts>
using map = typename map_impl<F, Ts>::type;

// --------------------------------------------------------------------------------

template <bool P, typename T, typename F>
struct choice_impl;

template <bool P, typename T, typename F>
using choice = typename choice_impl<P, T, F>::type;

// --------------------------------------------------------------------------------

// filter :: (a -> Bool) -> [a] -> [a]
// filter p [t|ts] =
//     if p t then t : rest else rest
//     where rest = filter (p) ts
// filter p [] = []

template <template <typename X> typename P, typename Ts>
struct filter_impl;

template <template <typename X> typename P, typename Ts>
using filter = typename filter_impl<P, Ts>::type;

// --------------------------------------------------------------------------------

// repeat :: Int -> a -> [a]
// repeat k t = t : repeat (k-1) t
// repeat 0 t = []

template <usize K, typename T>
struct repeat_impl;

template <usize K, typename T>
using repeat = typename repeat_impl<K, T>::type;

// --------------------------------------------------------------------------------

} // namespace vixen

#include "typeops.inl"