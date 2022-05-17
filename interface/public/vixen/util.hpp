#pragma once

#include "vixen/traits.hpp"
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

#define VIXEN_DEFAULT_ALL(classname) \
    VIXEN_DEFAULT_MOVE(classname)    \
    VIXEN_DEFAULT_COPY(classname)

#define VIXEN_MOVE_ONLY(classname) \
    VIXEN_DEFAULT_MOVE(classname)  \
    VIXEN_DELETE_COPY(classname)

#define VIXEN_UNMOVABLE(classname) \
    VIXEN_DELETE_COPY(classname)   \
    VIXEN_DELETE_MOVE(classname)

#define loop while (true)
#define mv(x) ::vixen::util::move(x)

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define _STATIC_INIT_ID() _vixen_static_runner__##__COUNTER__

#define RUN_AT_STATIC_INIT(...)                                            \
    static auto _STATIC_INIT_ID() = ::vixen::impl::run_at_static_init([] { \
        __VA_ARGS__;                                                       \
    });

namespace vixen::impl {
template <typename F>
struct run_at_static_init {
    run_at_static_init(F func) { func(); }
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

// TODO: use cheri functions/llvm intrinsits/whatever here instead of always just being NOPs

// remember, kids! pointers are not just numbers! But if you want to act like they are anyways, here
// you go, i guess.
/// @ingroup vixen_util
template <typename T>
constexpr usize addressOf(T *ptr) noexcept {
    return reinterpret_cast<usize>(ptr);
}

/// @ingroup vixen_util
template <typename T>
constexpr T *constructPointer(T *provenance, usize address) noexcept {
    return reinterpret_cast<T *>(address);
}

/// @ingroup vixen_util
template <typename P>
constexpr P alignPointerUp(P ptr, usize align) noexcept {
    auto mask = align - 1;
    auto addr = addressOf(ptr);
    auto alignedAddr = (addr + mask) & ~mask;
    return constructPointer(ptr, alignedAddr);
}

/// @ingroup vixen_util
template <typename P>
constexpr P alignPointerDown(P ptr, usize align) noexcept {
    auto mask = align - 1;
    auto addr = addressOf(ptr);
    auto alignedAddr = addr & ~mask;
    return constructPointer(ptr, alignedAddr);
}

/// @ingroup vixen_util
template <typename P>
constexpr bool isAligned(P ptr, usize align) noexcept {
    auto mask = align - 1;
    return (addressOf(ptr) & mask) == 0;
}

/// @ingroup vixen_util
constexpr void *offsetRaw(void *ptr, isize bytes) noexcept {
    return static_cast<void *>(static_cast<opaque *>(ptr) + bytes);
}

/// @ingroup vixen_util
constexpr void const *offsetRaw(void const *ptr, isize bytes) noexcept {
    return static_cast<void const *>(static_cast<opaque const *>(ptr) + bytes);
}

/// @ingroup vixen_util
constexpr usize alignUp(usize toAlign, usize alignment) noexcept {
    auto mask = alignment - 1;
    return (toAlign + mask) & ~mask;
}

/// @ingroup vixen_util
constexpr usize alignDown(usize toAlign, usize alignment) noexcept {
    auto mask = alignment - 1;
    return toAlign & ~mask;
}

template <typename T>
bool doRegionsOverlap(T *a, usize lengthA, T *b, usize lengthB) noexcept {
    usize loA = addressOf(a);
    usize hiA = loA + lengthA;
    usize loB = addressOf(b);
    usize hiB = loB + lengthB;

    bool loAInB = (loA >= loB) & (loA < hiB);
    bool hiAInB = (hiA >= loB) & (hiA < hiB);
    bool loBInA = (loB >= loA) & (loB < hiA);
    bool hiBInA = (hiB >= loA) & (hiB < hiA);

    return loAInB | hiAInB | loBInA | hiBInA;
}

/**
 * @ingroup vixen_util
 * @brief copy-assign a non-overlapping sequence of elements into another sequence.
 *
 * @pre the range `[src, src + count)` must contain `count` elements of type `T`.
 * @pre the range `[dst, dst + count)` must contain `count` elements of type `T`.
 * @pre the ranges `[src, src + count)` and `[dst, dst + count)` must not overlap.
 *
 * @param count the number of elements to copy.
 */
template <typename T>
inline void arrayCopyNonoverlapping(T const *src, T *dst, usize count) {
    if constexpr (std::is_trivial_v<T>) {
        ::std::memcpy(dst, src, sizeof(T) * count);
    } else {
        for (usize i = 0; i < count; ++i) {
            dst[i] = src[i];
        }
    }
}

/**
 * @ingroup vixen_util
 * @brief copy-assign a possibly-overlapping sequence of elements into another sequence.
 *
 * @pre the range `[src, src + count)` must contain `count` elements of type `T`.
 * @pre the range `[dst, dst + count)` must contain `count` elements of type `T`.
 *
 * @param count the number of elements to copy.
 */
template <typename T>
inline void arrayCopy(T const *src, T *dst, usize count) {
    if constexpr (std::is_trivial_v<T>) {
        ::std::memmove(dst, src, sizeof(T) * count);
    } else {
        if (src > dst) {
            for (usize i = 0; i < count; ++i) {
                dst[i] = src[i];
            }
        } else if (src < dst) {
            for (usize i = 0; i < count; ++i) {
                dst[count - i - 1] = src[count - i - 1];
            }
        }
    }
}

/**
 * @ingroup vixen_util
 * @brief copy-construct a non-overlapping sequence of elements into uninitialized memory.
 *
 * @pre the range `[src, src + count)` must contain `count` elements of type `T`.
 * @pre the range `[dst, dst + count)` must be large enough to fit `count` elements of type `T`.
 * @pre the ranges `[src, src + count)` and `[dst, dst + count)` must not overlap.
 *
 * @param count the number of elements to copy.
 */
template <typename T>
inline void arrayCopyUninitializedNonoverlapping(T const *src, T *dst, usize count) {
    if constexpr (std::is_trivial_v<T>) {
        ::std::memcpy(dst, src, sizeof(T) * count);
    } else {
        for (usize i = 0; i < count; ++i) {
            new (std::addressof(dst[i])) T{src[i]};
        }
    }
}

/**
 * @ingroup vixen_util
 * @brief copy-construct a potentially-overlapping sequence of elements into uninitialized memory.
 *
 * @pre the range `[src, src + count)` must contain `count` elements of type `T`.
 * @pre the range `[dst, dst + count)` must be large enough to fit `count` elements of type `T`.
 *
 * @param count the number of elements to copy.
 */
template <typename T>
inline void arrayCopyUninitialized(T const *src, T *dst, usize count) {
    if constexpr (std::is_trivial_v<T>) {
        ::std::memmove(dst, src, sizeof(T) * count);
    } else {
        if (src > dst) {
            for (usize i = 0; i < count; ++i) {
                new (std::addressof(dst[i])) T{src[i]};
            }
        } else if (src < dst) {
            for (usize i = 0; i < count; ++i) {
                new (std::addressof(dst[count - i - 1])) T{src[count - i - 1]};
            }
        }
    }
}

/**
 * @ingroup vixen_util
 * @brief move-assign a non-overlapping sequence of elements into another sequence.
 *
 * @pre the range `[src, src + count)` must contain `count` elements of type `T`.
 * @pre the range `[dst, dst + count)` must contain `count` elements of type `T`.
 * @pre the ranges `[src, src + count)` and `[dst, dst + count)` must not overlap.
 *
 * @param count the number of elements to move.
 */
template <typename T>
inline void arrayMoveNonoverlapping(T *src, T *dst, usize count) {
    if constexpr (std::is_trivial_v<T>) {
        ::std::memcpy(dst, src, sizeof(T) * count);
    } else {
        for (usize i = 0; i < count; ++i) {
            dst[i] = mv(src[i]);
        }
    }
}

/**
 * @ingroup vixen_util
 * @brief move-assign a possibly-overlapping sequence of elements into another sequence.
 *
 * @pre the range `[src, src + count)` must contain `count` elements of type `T`.
 * @pre the range `[dst, dst + count)` must contain `count` elements of type `T`.
 *
 * @param count the number of elements to move.
 */
template <typename T>
inline void arrayMove(T *src, T *dst, usize count) {
    if constexpr (std::is_trivial_v<T>) {
        ::std::memmove(dst, src, sizeof(T) * count);
    } else {
        if (src > dst) {
            for (usize i = 0; i < count; ++i) {
                dst[i] = mv(src[i]);
            }
        } else if (src < dst) {
            for (usize i = 0; i < count; ++i) {
                dst[count - i - 1] = mv(src[count - i - 1]);
            }
        }
    }
}

/**
 * @ingroup vixen_util
 * @brief move-construct a non-overlapping sequence of elements into uninitialized memory.
 *
 * @pre the range `[src, src + count)` must contain `count` elements of type `T`.
 * @pre the range `[dst, dst + count)` must be large enough to fit `count` elements of type `T`.
 * @pre the ranges `[src, src + count)` and `[dst, dst + count)` must not overlap.
 *
 * @param count the number of elements to move.
 */
template <typename T>
inline void arrayMoveUninitializedNonoverlapping(T *src, T *dst, usize count) {
    if constexpr (std::is_trivial_v<T>) {
        ::std::memcpy(dst, src, sizeof(T) * count);
    } else {
        for (usize i = 0; i < count; ++i) {
            new (std::addressof(dst[i])) T{mv(src[i])};
        }
    }
}

/**
 * @ingroup vixen_util
 * @brief move-construct a potentially-overlapping sequence of elements into uninitialized memory.
 *
 * @pre the range `[src, src + count)` must contain `count` elements of type `T`.
 * @pre the range `[dst, dst + count)` must be large enough to fit `count` elements of type `T`.
 *
 * @param count the number of elements to move.
 */
template <typename T>
inline void arrayMoveUninitialized(T *src, T *dst, usize count) {
    if constexpr (std::is_trivial_v<T>) {
        ::std::memmove(dst, src, sizeof(T) * count);
    } else {
        if (src > dst) {
            for (usize i = 0; i < count; ++i) {
                new (std::addressof(dst[i])) T{mv(src[i])};
            }
        } else if (src < dst) {
            for (usize i = 0; i < count; ++i) {
                new (std::addressof(dst[count - i - 1])) T{mv(src[count - i - 1])};
            }
        }
    }
}

/// @ingroup vixen_util
template <typename T>
inline void arrayCopyToRaw(T const *src, void *dst, usize elements) noexcept {
    static_assert(std::is_trivial_v<T>);
    ::std::memmove(dst, src, sizeof(T) * elements);
}

/// @ingroup vixen_util
template <typename T>
inline void arrayCopyNonoverlappingToRaw(T const *src, void *dst, usize elements) noexcept {
    static_assert(std::is_trivial_v<T>);
    ::std::memcpy(dst, src, sizeof(T) * elements);
}

/// @ingroup vixen_util
template <typename T>
inline void arrayCopyFromRaw(void const *src, T *dst, usize elements) noexcept {
    static_assert(std::is_trivial_v<T>);
    ::std::memmove(dst, src, sizeof(T) * elements);
}

/// @ingroup vixen_util
template <typename T>
inline void arrayCopyNonoverlappingFromRaw(void const *src, T *dst, usize elements) noexcept {
    static_assert(std::is_trivial_v<T>);
    ::std::memcpy(dst, src, sizeof(T) * elements);
}

/// @ingroup vixen_util
inline void arrayCopyRaw(void const *src, void *dst, usize bytes) noexcept {
    ::std::memmove(dst, src, bytes);
}

/// @ingroup vixen_util
inline void arrayCopyNonoverlappingRaw(void const *src, void *dst, usize bytes) noexcept {
    ::std::memcpy(dst, src, bytes);
}

/// @ingroup vixen_util
template <typename T>
constexpr void fillUninitialized(T const &pattern, T *ptr, usize count) {
    // not calling memset here cuz its not flexible enough in this case.
    for (usize i = 0; i < count; i++) {
        new (ptr + i) T(pattern);
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
constexpr void constructInPlace(T *location, Args &&...args) {
    new (static_cast<void *>(location)) T(std::forward<Args>(args)...);
}

/// @ingroup vixen_util
template <typename T>
constexpr bool isPowerOfTwo(T n) noexcept {
    if (n == T(0)) return false;
    return (n & (n - T(1))) == T(0);
}

/// @ingroup vixen_util
template <typename T>
constexpr T &readRawAs(void *ptr) noexcept {
    return *static_cast<T *>(ptr);
}

/// @ingroup vixen_util
template <typename T>
constexpr T const &readRawAs(void const *ptr) noexcept {
    return *static_cast<const T *>(ptr);
}

/// @ingroup vixen_util
constexpr isize rawptr_difference(void *lo, void *hi) noexcept {
    return reinterpret_cast<isize>(hi) - reinterpret_cast<isize>(lo);
}

/// @ingroup vixen_util
template <typename T>
constexpr T const &min(T const &a, T const &b) noexcept {
    return a > b ? b : a;
}

/// @ingroup vixen_util
template <typename T>
constexpr T &min(T &a, T &b) noexcept {
    return a > b ? b : a;
}

/// @ingroup vixen_util
template <typename T>
constexpr T const &max(T const &a, T const &b) noexcept {
    return a > b ? a : b;
}

/// @ingroup vixen_util
template <typename T>
constexpr T &max(T &a, T &b) noexcept {
    return a > b ? a : b;
}

/// @ingroup vixen_util
template <typename T>
constexpr T const &clamp(T const &value, T const &lo, T const &hi) noexcept {
    return value < lo ? lo : (value > hi ? hi : value);
}

/// @ingroup vixen_util
template <typename T>
constexpr T &clamp(T &value, T &lo, T &hi) noexcept {
    return value < lo ? lo : (value > hi ? hi : value);
}

} // namespace vixen::util
