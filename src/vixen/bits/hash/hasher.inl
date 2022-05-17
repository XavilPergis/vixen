#pragma once

#include "vixen/traits.hpp"
#include "vixen/types.hpp"
#include "vixen/util.hpp"

namespace vixen {

#pragma region "Hasher"
// + ----- Hasher --------------------------------------------------------------- +

constexpr u64 rol(const u64 n, const u64 i) {
    const u64 m = std::numeric_limits<u64>::digits - 1;
    const u64 c = i & m;
    return (n << c) | (n >> ((-c) & m));
}

constexpr u64 ror(const u64 n, const u64 i) {
    const u64 m = std::numeric_limits<u64>::digits - 1;
    const u64 c = i & m;
    return (n >> c) | (n << ((-c) & m));
}

namespace detail {

constexpr u64 rotation = 5;
constexpr u64 key64 = 0x517cc1b727220a95ul;
constexpr u32 key32 = 0x9e3779b9u;

constexpr void fxHasherAddHash(u64 &cur, u64 word) {
    cur = word ^ rol(cur, rotation);
    cur *= key64;
}

constexpr void fxHasherAddHash(u32 &cur, u32 word) {
    cur = word ^ rol(cur, rotation);
    cur *= key32;
}

constexpr u64 hashFinalize(u64 v) {
    v ^= ror(v, 25) ^ ror(v, 50);
    v *= 0xa24baed4963ee407ul;
    v ^= ror(v, 24) ^ ror(v, 49);
    v *= 0x9fb21c651e98df25ul;
    return v ^ v >> 28;
}

} // namespace detail

constexpr FxHasher::FxHasher(u64 seed) noexcept : current(seed) {}

constexpr u64 FxHasher::finish() noexcept {
    return detail::hashFinalize(current);
}

// @FIXME: this has bugs! i was getting inconsistent hash values for strings, even on the first
// character, so i suspect a pointer value is getting mixed up in the hash somewhere. It could also
// be the case that we dont handle alignment correctly.
// UPDATE: it was UB >n<
// constexpr void FxHasher::writeBytes(char const *data, usize len) noexcept {
//     void const *aligned = util::alignPointerUp(data, alignof(u64));
//     void const *cur = data;
//     void const *end = util::offsetRaw(data, len);

//     while (cur < aligned) {
//         detail::fxHasherAddHash(current, (u64)util::readRawAs<u8>(cur));
//         cur = util::offsetRaw(cur, 1);
//     }

//     while (util::offsetRaw(cur, sizeof(u64)) < end) {
//         detail::fxHasherAddHash(current, util::readRawAs<u64>(cur));
//         cur = util::offsetRaw(cur, sizeof(u64));
//     }

//     while (cur < end) {
//         detail::fxHasherAddHash(current, (u64)util::readRawAs<u8>(cur));
//         cur = util::offsetRaw(cur, 1);
//     }
// }

#define _VIXEN_FXHASHER_WRITE_OVERLOAD(T)               \
    template <>                                         \
    constexpr void FxHasher::write<T>(const T &value) { \
        detail::fxHasherAddHash(current, (u64)(value)); \
    }

// Add fast-path specializations for small key types, calling `add_to_hash` directly.
// _VIXEN_FXHASHER_WRITE_OVERLOAD(f32)
// _VIXEN_FXHASHER_WRITE_OVERLOAD(f64)
// _VIXEN_FXHASHER_WRITE_OVERLOAD(u64)
// _VIXEN_FXHASHER_WRITE_OVERLOAD(u32)
// _VIXEN_FXHASHER_WRITE_OVERLOAD(u16)
// _VIXEN_FXHASHER_WRITE_OVERLOAD(u8)
// _VIXEN_FXHASHER_WRITE_OVERLOAD(i64)
// _VIXEN_FXHASHER_WRITE_OVERLOAD(i32)
// _VIXEN_FXHASHER_WRITE_OVERLOAD(i16)
// _VIXEN_FXHASHER_WRITE_OVERLOAD(i8)
// _VIXEN_FXHASHER_WRITE_OVERLOAD(char)

constexpr void FxHasher::writeU8(u8 value) noexcept {
    detail::fxHasherAddHash(current, static_cast<u32>(value));
}
constexpr void FxHasher::writeU16(u16 value) noexcept {
    detail::fxHasherAddHash(current, static_cast<u32>(value));
}
constexpr void FxHasher::writeU32(u32 value) noexcept {
    detail::fxHasherAddHash(current, static_cast<u32>(value));
}
constexpr void FxHasher::writeU64(u64 value) noexcept {
    detail::fxHasherAddHash(current, static_cast<u64>(value));
}
constexpr void FxHasher::writeUSize(usize value) noexcept {
    detail::fxHasherAddHash(current, static_cast<u64>(value));
}
constexpr void FxHasher::writeI8(i8 value) noexcept {
    detail::fxHasherAddHash(current, static_cast<u32>(value));
}
constexpr void FxHasher::writeI16(i16 value) noexcept {
    detail::fxHasherAddHash(current, static_cast<u32>(value));
}
constexpr void FxHasher::writeI32(i32 value) noexcept {
    detail::fxHasherAddHash(current, static_cast<u32>(value));
}
constexpr void FxHasher::writeI64(i64 value) noexcept {
    detail::fxHasherAddHash(current, static_cast<u64>(value));
}
constexpr void FxHasher::writeISize(isize value) noexcept {
    detail::fxHasherAddHash(current, static_cast<u64>(value));
}
constexpr void FxHasher::writeBytes(char const *data, usize len) noexcept {
    while (*data != 0) {
        writeU8(*data);
        data += 1;
    }
}

template <typename H, typename T>
constexpr u64 makeHash(const T &value) noexcept {
    H hasher{};
    hash(value, hasher);
    return hasher.finish();
}

template <typename H, typename T>
constexpr u64 makeHash(const T &value, H &hasher) noexcept {
    hash(value, hasher);
    return hasher.finish();
}

#pragma endregion

} // namespace vixen
