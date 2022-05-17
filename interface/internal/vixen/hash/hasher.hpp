#pragma once

#include "vixen/slice.hpp"
#include "vixen/types.hpp"

namespace vixen {

// dumb dumb simple shitty hasher
struct FxHasher {
    constexpr FxHasher(u64 seed = 0);

    constexpr u64 finish();
    constexpr void write_bytes(void const *data, usize len);

    template <typename T>
    constexpr void write(T const &value);

    u64 current;
};

template <typename H, typename T>
constexpr u64 make_hash(T const &value);

template <typename H, typename T>
constexpr u64 make_hash(T const &value, H &hasher);

using default_hasher = FxHasher;

} // namespace vixen

#include "vixen/bits/hash/hasher.inl"
