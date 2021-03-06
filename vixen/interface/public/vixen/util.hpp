#pragma once

#include "vixen/types.hpp"

#include <cstdlib>
#include <cstring>

/// @defgroup vixen_util Utility
/// @brief Miscellaneous utilities

#define VIXEN_NODISCARD [[nodiscard]]

#define loop while (true)
#define mv(x) ::vixen::util::move(x)

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define _STATIC_INIT_ID() __static_runner_##__COUNTER__

#define RUN_AT_STATIC_INIT(...)                                            \
    static auto _STATIC_INIT_ID() = ::vixen::impl::run_at_static_init([] { \
        __VA_ARGS__                                                        \
    })

namespace vixen::impl {
template <typename F>
struct run_at_static_init {
    run_at_static_init(F func) {
        func();
    }
};
} // namespace vixen::impl

namespace vixen::util {
/// @ingroup vixen_util
template <typename T>
inline void copy(T const *src, T *dst, usize elements) {
    ::std::memmove(dst, src, sizeof(T) * elements);
}

/// @ingroup vixen_util
template <typename T>
inline void copy_nonoverlapping(T const *src, T *dst, usize elements) {
    ::std::memcpy(dst, src, sizeof(T) * elements);
}

/// @ingroup vixen_util
template <typename T>
inline void copy_to_raw(T const *src, void *dst, usize elements) {
    ::std::memmove(dst, src, sizeof(T) * elements);
}

/// @ingroup vixen_util
template <typename T>
inline void copy_nonoverlapping_to_raw(T const *src, void *dst, usize elements) {
    ::std::memcpy(dst, src, sizeof(T) * elements);
}

/// @ingroup vixen_util
template <typename T>
inline void copy_from_raw(void const *src, T *dst, usize elements) {
    ::std::memmove(dst, src, sizeof(T) * elements);
}

/// @ingroup vixen_util
template <typename T>
inline void copy_nonoverlapping_from_raw(void const *src, T *dst, usize elements) {
    ::std::memcpy(dst, src, sizeof(T) * elements);
}

template <typename T>
struct remove_reference_trait {
    using type = T;
};

template <typename T>
struct remove_reference_trait<T &> : public remove_reference_trait<T> {};
template <typename T>
struct remove_reference_trait<T &&> : public remove_reference_trait<T> {};

template <typename T>
using remove_reference = typename remove_reference_trait<T>::type;

/// @ingroup vixen_util
template <typename T>
inline remove_reference<T> &&move(T &&val) {
    return static_cast<remove_reference<T> &&>(val);
}

/// @ingroup vixen_util
template <typename T>
inline void fill(T const &pattern, T *ptr, usize count) {
    for (usize i = 0; i < count; i++) {
        ptr[i] = pattern;
    }
}

/// @ingroup vixen_util
template <typename T, typename U = T>
inline T exchange(T &old_val, U &&new_val) {
    auto ret = std::move(old_val);
    old_val = std::forward<U>(new_val);
    return ret;
}

/// @ingroup vixen_util
template <typename T, typename... Args>
inline void construct_in_place(T *location, Args &&...args) {
    new (static_cast<rawptr>(location)) T(std::forward<Args>(args)...);
}

/// @ingroup vixen_util
template <typename T>
constexpr bool is_power_of_two(T n) {
    return (n != 0) && ((n & (n - 1)) == 0);
}

/// @ingroup vixen_util
template <typename P, typename A>
constexpr P align_pointer_up(P ptr, A align) {
    return (P)((A)ptr + (align - 1) & ~(align - 1));
}

/// @ingroup vixen_util
constexpr rawptr offset_rawptr(rawptr ptr, isize bytes) {
    return (rawptr)((isize)ptr + bytes);
}

/// @ingroup vixen_util
constexpr const_rawptr offset_rawptr(const_rawptr ptr, isize bytes) {
    return (const_rawptr)((isize)ptr + bytes);
}

/// @ingroup vixen_util
template <typename T>
constexpr T &read_rawptr(rawptr ptr) {
    return *static_cast<T *>(ptr);
}

/// @ingroup vixen_util
template <typename T>
constexpr T const &read_rawptr(const_rawptr ptr) {
    return *static_cast<const T *>(ptr);
}

/// @ingroup vixen_util
constexpr isize rawptr_difference(rawptr lo, rawptr hi) {
    return reinterpret_cast<isize>(hi) - reinterpret_cast<isize>(lo);
}

} // namespace vixen::util
