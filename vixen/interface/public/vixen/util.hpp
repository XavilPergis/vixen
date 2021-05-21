#pragma once

#include "vixen/types.hpp"

#include <cstdlib>
#include <cstring>
#include <utility>

/// @defgroup vixen_util Utility
/// @brief Miscellaneous utilities

#define VIXEN_NODISCARD [[nodiscard]]

#define VIXEN_DELETE_COPY(classname)       \
    classname(classname const &) = delete; \
    classname &operator=(classname const &) = delete;

#define VIXEN_DELETE_MOVE(classname)  \
    classname(classname &&) = delete; \
    classname &operator=(classname &&) = delete;

#define VIXEN_DEFAULT_COPY(classname)       \
    classname(classname const &) = default; \
    classname &operator=(classname const &) = default;

#define VIXEN_DEFAULT_MOVE(classname)  \
    classname(classname &&) = default; \
    classname &operator=(classname &&) = default;

#define VIXEN_DEFAULT_MOVE_DELETE_COPY(classname)     \
    classname(classname const &) = delete;            \
    classname &operator=(classname const &) = delete; \
    classname(classname &&) = default;                \
    classname &operator=(classname &&) = default;

#define loop while (true)
#define mv(x) ::vixen::util::move(x)

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define _STATIC_INIT_ID() __static_runner_##__COUNTER__

#define RUN_AT_STATIC_INIT(...)                                            \
    static auto _STATIC_INIT_ID() = ::vixen::impl::run_at_static_init([] { \
        __VA_ARGS__;                                                       \
    });

namespace vixen::impl {
template <typename F>
struct run_at_static_init {
    run_at_static_init(F func) {
        func();
    }
};
} // namespace vixen::impl

namespace vixen::util {

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
constexpr remove_reference<T> &&move(T &&val) {
    return static_cast<remove_reference<T> &&>(val);
}

/// @ingroup vixen_util
template <typename T>
inline void copy(T const *src, T *dst, usize elements) {
    if constexpr (std::is_trivially_copyable_v<T>) {
        ::std::memmove(dst, src, sizeof(T) * elements);
    } else {
        for (usize i = 0; i < elements; ++i) {
            dst[i] = src[i];
        }
    }
}

/// @ingroup vixen_util
template <typename T>
inline void copy_move(T *src, T *dst, usize elements) {
    if constexpr (std::is_trivially_copyable_v<T>) {
        ::std::memmove(dst, src, sizeof(T) * elements);
    } else {
        if (src > dst) {
            for (usize i = 0; i < elements; ++i) {
                dst[i] = mv(src[i]);
            }
        } else if (src < dst) {
            for (usize i = 0; i < elements; ++i) {
                dst[i] = mv(src[i]);
            }
        }
    }
}

/// @ingroup vixen_util
template <typename T>
inline void copy_nonoverlapping(T const *src, T *dst, usize elements) {
    if constexpr (std::is_trivially_copyable_v<T>) {
        ::std::memcpy(dst, src, sizeof(T) * elements);
    } else {
        for (usize i = 0; i < elements; ++i) {
            dst[i] = src[i];
        }
    }
}

/// @ingroup vixen_util
template <typename T>
inline void copy_move_nonoverlapping(T const *src, T *dst, usize elements) {
    if constexpr (std::is_trivially_copyable_v<T>) {
        ::std::memcpy(dst, src, sizeof(T) * elements);
    } else {
        for (usize i = 0; i < elements; ++i) {
            dst[i] = mv(src[i]);
        }
    }
}

/// @ingroup vixen_util
template <typename T>
inline void copy_to_raw(T const *src, void *dst, usize elements) {
    static_assert(std::is_trivially_copyable_v<T>);
    ::std::memmove(dst, src, sizeof(T) * elements);
}

/// @ingroup vixen_util
template <typename T>
inline void copy_nonoverlapping_to_raw(T const *src, void *dst, usize elements) {
    static_assert(std::is_trivially_copyable_v<T>);
    ::std::memcpy(dst, src, sizeof(T) * elements);
}

/// @ingroup vixen_util
template <typename T>
inline void copy_from_raw(void const *src, T *dst, usize elements) {
    static_assert(std::is_trivially_copyable_v<T>);
    ::std::memmove(dst, src, sizeof(T) * elements);
}

/// @ingroup vixen_util
template <typename T>
inline void copy_nonoverlapping_from_raw(void const *src, T *dst, usize elements) {
    static_assert(std::is_trivially_copyable_v<T>);
    ::std::memcpy(dst, src, sizeof(T) * elements);
}

/// @ingroup vixen_util
template <typename T>
constexpr void fill(T const &pattern, T *ptr, usize count) {
    for (usize i = 0; i < count; i++) {
        ptr[i] = pattern;
    }
}

/// @ingroup vixen_util
template <typename T, typename U>
constexpr T exchange(T &old_val, U &&new_val) {
    auto ret = move(old_val);
    old_val = std::forward<U>(new_val);
    return ret;
}

/// @ingroup vixen_util
template <typename T>
constexpr void swap(T &lhs, T &rhs) {
    T tmp(mv(lhs));
    lhs = mv(rhs);
    rhs = mv(tmp);
}

/// @ingroup vixen_util
template <typename T, typename... Args>
constexpr void construct_in_place(T *location, Args &&...args) {
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
    return (P)(((A)ptr + (align - 1)) & ~(align - 1));
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

/// @ingroup vixen_util
template <typename T>
constexpr T const &min(T const &a, T const &b) {
    return a > b ? b : a;
}

/// @ingroup vixen_util
template <typename T>
constexpr T &min(T &a, T &b) {
    return a > b ? b : a;
}

/// @ingroup vixen_util
template <typename T>
constexpr T const &max(T const &a, T const &b) {
    return a > b ? a : b;
}

/// @ingroup vixen_util
template <typename T>
constexpr T &max(T &a, T &b) {
    return a > b ? a : b;
}

/// @ingroup vixen_util
template <typename T>
constexpr T const &clamp(T const &value, T const &lo, T const &hi) {
    return value < lo ? lo : (value > hi ? hi : value);
}

/// @ingroup vixen_util
template <typename T>
constexpr T &clamp(T &value, T &lo, T &hi) {
    return value < lo ? lo : (value > hi ? hi : value);
}

} // namespace vixen::util
