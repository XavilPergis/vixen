#pragma once

#include "vixen/defines.hpp"
#include "vixen/log.hpp"

#define _VIXEN_BOUNDS_CHECK(idx, len)                                 \
    VIXEN_DEBUG_ASSERT(idx < len,                                     \
        "Index out of bounds: The length is {} but the index is {}.", \
        len,                                                          \
        idx)
#define _VIXEN_BOUNDS_CHECK_EXCLUSIVE(idx, len)                       \
    VIXEN_DEBUG_ASSERT(idx <= len,                                    \
        "Index out of bounds: The length is {} but the index is {}.", \
        len,                                                          \
        idx)

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

#define VIXEN_PANIC(...)                                       \
    do {                                                       \
        VIXEN_CRITICAL_EXT(::vixen::panic_logger, __VA_ARGS__) \
        ::vixen::detail::invoke_panic_handler();               \
        __builtin_unreachable();                               \
    } while (false)

#ifdef _VIXEN_ASSERT_ENABLED
#define VIXEN_ASSERT(cond, ...)       \
    do {                              \
        if (!(likely(cond)))          \
            VIXEN_PANIC(__VA_ARGS__); \
    } while (false)
#else
#define VIXEN_ASSERT(...)
#endif

// Debug assertions are for safety checks in hot code that we wouldn't want to be slowed down in
// release mode, like bounds checking on index operators. Normal asserts are preferred to these
// wherever you can put them, though.
#ifdef _VIXEN_DEBUG_ASSERT_ENABLED
#define VIXEN_DEBUG_ASSERT(...) VIXEN_ASSERT(__VA_ARGS__)
#else
#define VIXEN_DEBUG_ASSERT(...)
#endif

#define VIXEN_UNREACHABLE(...)                                         \
    do {                                                               \
        VIXEN_ASSERT(false, "Entered unreachable code: " __VA_ARGS__); \
        __builtin_unreachable();                                       \
    } while (false)

namespace vixen {

extern logger_id panic_logger;

using panic_handler = void (*)();

panic_handler install_panic_hanlder(panic_handler handler);
panic_handler remove_panic_handler();

namespace detail {
void invoke_panic_handler();
} // namespace detail

} // namespace vixen
