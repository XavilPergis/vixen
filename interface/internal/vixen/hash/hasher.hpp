#pragma once

#include "vixen/slice.hpp"
#include "vixen/traits.hpp"
#include "vixen/types.hpp"

namespace vixen {

// clang-format off
template <typename H>
concept Hasher = requires(H &hasher) {
    requires requires(u64 seed) { {H(seed)} -> IsSame<H>; };
    {hasher.finish()} -> IsSame<u64>;

    requires requires(u8 value)    { hasher.writeU8(value);    };
    requires requires(u16 value)   { hasher.writeU16(value);   };
    requires requires(u32 value)   { hasher.writeU32(value);   };
    requires requires(u64 value)   { hasher.writeU64(value);   };
    requires requires(usize value) { hasher.writeUSize(value); };
    requires requires(i8 value)    { hasher.writeI8(value);    };
    requires requires(i16 value)   { hasher.writeI16(value);   };
    requires requires(i32 value)   { hasher.writeI32(value);   };
    requires requires(i64 value)   { hasher.writeI64(value);   };
    requires requires(isize value) { hasher.writeISize(value); };
};

template <typename T, typename H>
concept Hashable = Hasher<H> && requires(const T &value, H &hasher) {
    {hash(value, hasher)};
};
// clang-format on

// dumb dumb simple shitty hasher
struct FxHasher {
    constexpr FxHasher(u64 seed = 0) noexcept;

    constexpr u64 finish() noexcept;

    constexpr void writeU8(u8 value) noexcept;
    constexpr void writeU16(u16 value) noexcept;
    constexpr void writeU32(u32 value) noexcept;
    constexpr void writeU64(u64 value) noexcept;
    constexpr void writeUSize(usize value) noexcept;
    constexpr void writeI8(i8 value) noexcept;
    constexpr void writeI16(i16 value) noexcept;
    constexpr void writeI32(i32 value) noexcept;
    constexpr void writeI64(i64 value) noexcept;
    constexpr void writeISize(isize value) noexcept;
    constexpr void writeBytes(char const *data, usize len) noexcept;

    u64 current;
};

template <Hasher H, typename T>
constexpr u64 makeHash(T const &value) noexcept;

template <Hasher H, typename T>
constexpr u64 makeHash(T const &value, H &hasher) noexcept;

using default_hasher = FxHasher;

} // namespace vixen

#include "vixen/bits/hash/hasher.inl"
