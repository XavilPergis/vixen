#pragma once

#include "vixen/log.hpp"

#define _VIXEN_BOUNDS_CHECK(idx, len)                                 \
    VIXEN_ENGINE_DEBUG_ASSERT(idx < len,                              \
        "Index out of bounds: The length is {} but the index is {}.", \
        len,                                                          \
        idx)
#define _VIXEN_BOUNDS_CHECK_EXCLUSIVE(idx, len)                       \
    VIXEN_ENGINE_DEBUG_ASSERT(idx <= len,                             \
        "Index out of bounds: The length is {} but the index is {}.", \
        len,                                                          \
        idx)

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#if !defined(NDEBUG) || defined(VIXEN_FORCE_DEBUG)
#define VIXEN_IS_DEBUG
#endif

#if defined(VIXEN_FORCE_DISBLE_DEBUG_ASSERT) && defined(VIXEN_FORCE_ENABLE_DEBUG_ASSERT)
#error Debug assert cannot be both force enabled and force disabled at the same time! \
Try undefining either VIXEN_FORCE_DISBLE_DEBUG_ASSERT or VIXEN_FORCE_ENABLE_DEBUG_ASSERT.
#endif

#if defined(VIXEN_FORCE_ENABLE_DEBUG_ASSERT) && defined(VIXEN_FORCE_DISBLE_ASSERT)
#error Debug assert cannot be used without normal assertions! Try undefining either \
VIXEN_FORCE_ENABLE_DEBUG_ASSERT or VIXEN_FORCE_DISBLE_ASSERT.
#endif

// Only use debug asserts when in debug mode, unless they have been disabled by force.
#if defined(VIXEN_IS_DEBUG) && !defined(VIXEN_FORCE_DISBLE_DEBUG_ASSERT)
#define _VIXEN_DEBUG_ASSERT_ENABLED
#endif

// Always use asserts, unless the yare disabled by force.
#if !defined(VIXEN_FORCE_DISBLE_ASSERT)
#define _VIXEN_ASSERT_ENABLED
#endif

#ifdef _VIXEN_ASSERT_ENABLED
#define VIXEN_ASSERT(cond, ...)                          \
    if (!(likely(cond))) {                               \
        VIXEN_CRITICAL("Assertion Failed: " __VA_ARGS__) \
        ::vixen::detail::invoke_assert_handler();        \
        ::std::abort();                                  \
    }

#define VIXEN_ENGINE_ASSERT(cond, ...)                          \
    if (!(likely(cond))) {                                      \
        VIXEN_ENGINE_CRITICAL("Assertion Failed: " __VA_ARGS__) \
        ::vixen::detail::invoke_assert_handler();               \
        ::std::abort();                                         \
    }
#else
#define VIXEN_ASSERT(...)
#define VIXEN_ENGINE_ASSERT(...)
#endif

// Debug assertions are for safety checks in hot code that we wouldn't want to be slowed down in
// release mode, like bounds checking on index operators. Normal asserts are preferred to these
// wherever you can put them, though.
#ifdef _VIXEN_DEBUG_ASSERT_ENABLED
#define VIXEN_DEBUG_ASSERT(...) VIXEN_ASSERT(__VA_ARGS__)
#define VIXEN_ENGINE_DEBUG_ASSERT(...) VIXEN_ENGINE_ASSERT(__VA_ARGS__)
#else
#define VIXEN_DEBUG_ASSERT(...)
#define VIXEN_ENGINE_DEBUG_ASSERT(...)
#endif

#define VIXEN_UNREACHABLE(...) \
    VIXEN_ASSERT(false, "Entered unreachable code: " __VA_ARGS__) __builtin_unreachable();
#define VIXEN_ENGINE_UNREACHABLE(...) \
    VIXEN_ENGINE_ASSERT(false, "Entered unreachable code: " __VA_ARGS__) __builtin_unreachable();

namespace vixen {
using assert_handler = void (*)();

assert_handler install_assert_handler(assert_handler handler);
assert_handler remove_assert_handler();

namespace detail {
void invoke_assert_handler();
} // namespace detail

} // namespace vixen
