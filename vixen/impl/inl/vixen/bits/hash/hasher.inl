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

constexpr void fx_hasher_add_hash(u64 &cur, u64 word) {
    cur = word ^ rol(cur, rotation);
    cur *= key64;
}

constexpr void fx_hasher_add_hash(u32 &cur, u32 word) {
    cur = word ^ rol(cur, rotation);
    cur *= key32;
}

constexpr u64 hash_finalize(u64 v) {
    v ^= ror(v, 25) ^ ror(v, 50);
    v *= 0xa24baed4963ee407ul;
    v ^= ror(v, 24) ^ ror(v, 49);
    v *= 0x9fb21c651e98df25ul;
    return v ^ v >> 28;
}

}; // namespace detail

constexpr fx_hasher::fx_hasher(u64 seed) : current(seed) {}

constexpr u64 fx_hasher::finish() {
    return detail::hash_finalize(current);
}

constexpr void fx_hasher::write_bytes(const_rawptr data, usize len) {
    const_rawptr aligned = util::align_pointer_up(data, alignof(u64));
    const_rawptr cur = data;
    const_rawptr end = util::offset_rawptr(data, len);

    while (cur < aligned) {
        detail::fx_hasher_add_hash(current, (u64)util::read_rawptr<u8>(cur));
        cur = util::offset_rawptr(cur, 1);
    }

    while (util::offset_rawptr(cur, sizeof(u64)) < end) {
        detail::fx_hasher_add_hash(current, util::read_rawptr<u64>(cur));
        cur = util::offset_rawptr(cur, sizeof(u64));
    }

    while (cur < end) {
        detail::fx_hasher_add_hash(current, (u64)util::read_rawptr<u8>(cur));
        cur = util::offset_rawptr(cur, 1);
    }
}

#define _VIXEN_FXHASHER_WRITE_OVERLOAD(T)                  \
    template <>                                            \
    constexpr void fx_hasher::write<T>(const T &value) {   \
        detail::fx_hasher_add_hash(current, (u64)(value)); \
    }

// Add fast-path specializations for small key types, calling `add_to_hash` directly.
_VIXEN_FXHASHER_WRITE_OVERLOAD(f32)
_VIXEN_FXHASHER_WRITE_OVERLOAD(f64)
_VIXEN_FXHASHER_WRITE_OVERLOAD(u64)
_VIXEN_FXHASHER_WRITE_OVERLOAD(u32)
_VIXEN_FXHASHER_WRITE_OVERLOAD(u16)
_VIXEN_FXHASHER_WRITE_OVERLOAD(u8)
_VIXEN_FXHASHER_WRITE_OVERLOAD(i64)
_VIXEN_FXHASHER_WRITE_OVERLOAD(i32)
_VIXEN_FXHASHER_WRITE_OVERLOAD(i16)
_VIXEN_FXHASHER_WRITE_OVERLOAD(i8)

template <typename T>
constexpr void fx_hasher::write(const T &value) {
    static_assert(std::is_trivial_v<T>,
        "default fxhasher write can only be used with trivial types (define a custom hash specialization for T that only uses trivial types)");
    write_bytes(std::addressof(value), sizeof(T));
}

template <typename H, typename T>
constexpr u64 make_hash(const T &value) {
    H hasher{};
    hash(value, hasher);
    return hasher.finish();
}

template <typename H, typename T>
constexpr u64 make_hash(const T &value, H &hasher) {
    hash(value, hasher);
    return hasher.finish();
}

#pragma endregion

} // namespace vixen
