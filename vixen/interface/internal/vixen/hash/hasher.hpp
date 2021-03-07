#pragma once

#include "vixen/slice.hpp"
#include "vixen/types.hpp"

namespace vixen {

// dumb dumb simple shitty hasher
struct FxHasher {
    constexpr FxHasher(u64 seed = 0);

    constexpr u64 finish();
    constexpr void write_bytes(const_rawptr data, usize len);

    template <typename T>
    constexpr void write(const T &value);

    u64 current;
};

template <typename H, typename T>
constexpr u64 make_hash(const T &value);

template <typename H, typename T>
constexpr u64 make_hash(const T &value, H &hasher);

using default_hasher = FxHasher;

} // namespace vixen

#include "vixen/bits/hash/hasher.inl"
