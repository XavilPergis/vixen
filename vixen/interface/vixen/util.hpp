#pragma once

#include "vixen/types.hpp"

#include <cstdlib>
#include <cstring>

#define VIXEN_NODISCARD [[nodiscard]]

#define loop while (true)
#define MOVE(x) ::vixen::util::move(x)

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
template <typename T>
inline void copy(T const *src, T *dst, usize elements) {
    ::std::memmove(dst, src, sizeof(T) * elements);
}

template <typename T>
inline void copy_nonoverlapping(T const *src, T *dst, usize elements) {
    ::std::memcpy(dst, src, sizeof(T) * elements);
}

template <typename T>
inline void copy_to_raw(T const *src, void *dst, usize elements) {
    ::std::memmove(dst, src, sizeof(T) * elements);
}

template <typename T>
inline void copy_nonoverlapping_to_raw(T const *src, void *dst, usize elements) {
    ::std::memcpy(dst, src, sizeof(T) * elements);
}

template <typename T>
inline void copy_from_raw(void const *src, T *dst, usize elements) {
    ::std::memmove(dst, src, sizeof(T) * elements);
}

template <typename T>
inline void copy_nonoverlapping_from_raw(void const *src, T *dst, usize elements) {
    ::std::memcpy(dst, src, sizeof(T) * elements);
}

template <typename T>
inline typename std::remove_reference_t<T> &&move(T &&val) {
    return static_cast<typename std::remove_reference_t<T> &&>(val);
}

template <typename T>
inline void fill(T const &pattern, T *ptr, usize count) {
    for (usize i = 0; i < count; i++) {
        ptr[i] = pattern;
    }
}

template <typename T, typename U = T>
inline T exchange(T &old_val, U &&new_val) {
    auto ret = std::move(old_val);
    old_val = std::forward<U>(new_val);
    return ret;
}

template <typename T, typename... Args>
inline void construct_in_place(T *location, Args &&...args) {
    new (location) T(std::forward<Args>(args)...);
}

template <typename T>
constexpr bool is_power_of_two(T n) {
    return (n != 0) && ((n & (n - 1)) == 0);
}

} // namespace vixen::util
