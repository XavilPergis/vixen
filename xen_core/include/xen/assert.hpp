#pragma once

#include "xen/log.hpp"

#define _XEN_BOUNDS_CHECK(idx, len)                                   \
    XEN_ENGINE_DEBUG_ASSERT(idx < len,                                \
        "Index out of bounds: The length is {} but the index is {}.", \
        len,                                                          \
        idx)
#define _XEN_BOUNDS_CHECK_EXCLUSIVE(idx, len)                         \
    XEN_ENGINE_DEBUG_ASSERT(idx <= len,                               \
        "Index out of bounds: The length is {} but the index is {}.", \
        len,                                                          \
        idx)

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#if !defined(NDEBUG) || defined(XEN_FORCE_DEBUG)
#define XEN_IS_DEBUG
#endif

#if defined(XEN_FORCE_DISBLE_DEBUG_ASSERT) && defined(XEN_FORCE_ENABLE_DEBUG_ASSERT)
#error Debug assert cannot be both force enabled and force disabled at the same time! \
Try undefining either XEN_FORCE_DISBLE_DEBUG_ASSERT or XEN_FORCE_ENABLE_DEBUG_ASSERT.
#endif

#if defined(XEN_FORCE_ENABLE_DEBUG_ASSERT) && defined(XEN_FORCE_DISBLE_ASSERT)
#error Debug assert cannot be used without normal assertions! Try undefining either \
XEN_FORCE_ENABLE_DEBUG_ASSERT or XEN_FORCE_DISBLE_ASSERT.
#endif

// Only use debug asserts when in debug mode, unless they have been disabled by force.
#if defined(XEN_IS_DEBUG) && !defined(XEN_FORCE_DISBLE_DEBUG_ASSERT)
#define _XEN_DEBUG_ASSERT_ENABLED
#endif

// Always use asserts, unless the yare disabled by force.
#if !defined(XEN_FORCE_DISBLE_ASSERT)
#define _XEN_ASSERT_ENABLED
#endif

#ifdef _XEN_ASSERT_ENABLED
#define XEN_ASSERT(cond, ...)                          \
    if (!(likely(cond))) {                             \
        XEN_CRITICAL("Assertion Failed: " __VA_ARGS__) \
        ::xen::detail::invoke_assert_handler();        \
        ::std::abort();                                \
    }

#define XEN_ENGINE_ASSERT(cond, ...)                          \
    if (!(likely(cond))) {                                    \
        XEN_ENGINE_CRITICAL("Assertion Failed: " __VA_ARGS__) \
        ::xen::detail::invoke_assert_handler();               \
        ::std::abort();                                       \
    }
#else
#define XEN_ASSERT(...)
#define XEN_ENGINE_ASSERT(...)
#endif

// Debug assertions are for safety checks in hot code that we wouldn't want to be slowed down in
// release mode, like bounds checking on index operators. Normal asserts are preferred to these
// wherever you can put them, though.
#ifdef _XEN_DEBUG_ASSERT_ENABLED
#define XEN_DEBUG_ASSERT(...) XEN_ASSERT(__VA_ARGS__)
#define XEN_ENGINE_DEBUG_ASSERT(...) XEN_ENGINE_ASSERT(__VA_ARGS__)
#else
#define XEN_DEBUG_ASSERT(...)
#define XEN_ENGINE_DEBUG_ASSERT(...)
#endif

#define XEN_UNREACHABLE(...) \
    XEN_ASSERT(false, "Entered unreachable code: " __VA_ARGS__) __builtin_unreachable();
#define XEN_ENGINE_UNREACHABLE(...) \
    XEN_ENGINE_ASSERT(false, "Entered unreachable code: " __VA_ARGS__) __builtin_unreachable();

namespace xen {
using assert_handler = void (*)();

assert_handler install_assert_handler(assert_handler handler);
assert_handler remove_assert_handler();

namespace detail {
void invoke_assert_handler();
} // namespace detail

} // namespace xen
