#pragma once

#include "vixen/allocator/layout.hpp"
#include "vixen/allocator/profile.hpp"
#include "vixen/assert.hpp"
#include "vixen/types.hpp"
#include "vixen/util.hpp"

#include <cstddef>

/// @defgroup vixen_allocator Allocators
/// @brief Pluggable dynamic memory allocators.

/// @file
/// @ingroup vixen_allocator
/// @brief Main allocator file; defines the allocator interface, the global allocator, and various
/// allocator utilities.

// These prologs normalize the behavior of allocation and deallocation funtions
// and can provide extra correctness checks, so that this code is not duplicated
// in each allocatior implementation
#define _VIXEN_ALLOC_PROLOGUE(layout)                          \
    if ((layout).size == 0) {                                  \
        return nullptr;                                        \
    }                                                          \
    VIXEN_ASSERT(vixen::util::is_power_of_two((layout).align), \
        "Alignment {} is not a power of 2.",                   \
        (layout).align);

#define _VIXEN_DEALLOC_PROLOGUE(layout, ptr)                   \
    if (ptr == nullptr) {                                      \
        return;                                                \
    }                                                          \
    VIXEN_ASSERT(vixen::util::is_power_of_two((layout).align), \
        "Alignment {} is not a power of 2.",                   \
        (layout).align);

#define _VIXEN_REALLOC_PROLOGUE(old_layout, new_layout, ptr)       \
    if (ptr == nullptr) {                                          \
        return internal_alloc(new_layout);                         \
    }                                                              \
    if (old_layout == new_layout) {                                \
        return ptr;                                                \
    }                                                              \
    VIXEN_ASSERT(vixen::util::is_power_of_two((old_layout).align), \
        "Alignment {} is not a power of 2.",                       \
        (old_layout).align);                                       \
    VIXEN_ASSERT(vixen::util::is_power_of_two((new_layout).align), \
        "Alignment {} is not a power of 2.",                       \
        (new_layout).align);

namespace vixen::heap {

struct allocation_exception {};

constexpr u8 ALLOCATION_PATTERN = 0xae;
constexpr u8 DEALLOCATION_PATTERN = 0xfe;
constexpr usize MAX_LEGACY_ALIGNMENT = alignof(std::max_align_t);

// layout:
// An instance of layout (named "l") must satisfy:
//     - l.align must be a power of two unless l.size is 0, in which case l.align may also be 0.
//
// alloc:
// For an allocator request R:
//     - A is the allocator that the allocation request was submitted to.
//     - P is a pointer returned by R if R is successful.
//     - L is the layout associated with the allocation request.
//
//     - Arbitrary use must be allowed for any address in [P, P+L.size)
//     - P must be aligned to L.align.
//     - If R cannot be fulfilled, then an instance of AllocationException must be thrown.
//
// dealloc:
// For a deallocation request R:
//     - A is the allocator that the deallocation request was submitted to.
//     - P is the pointer associated with the deallocation request.
//     - L is the layout associated with the deallocation request.
//
//     - P must have been created with an allocation request to A.
//     - L must be equal to the layout that P was created with.
//     - R must not throw.

/// @ingroup vixen_allocator
/// @brief Base allocator class.
struct allocator {
    virtual ~allocator() {}

    VIXEN_NODISCARD void *alloc(const layout &layout);
    void dealloc(const layout &layout, void *ptr);
    VIXEN_NODISCARD void *realloc(
        const layout &old_layout, const layout &new_layout, void *old_ptr);

    /// @brief Unique ID of this allocator, used for allocator tracking.
    /// @see profile.hpp
    allocator_id id = NOT_TRACKED_ID;

protected:
    VIXEN_NODISCARD virtual void *internal_alloc(const layout &layout) = 0;
    virtual void internal_dealloc(const layout &layout, void *ptr) = 0;

    VIXEN_NODISCARD virtual void *internal_realloc(
        const layout &old_layout, const layout &new_layout, void *old_ptr);
};

/// @ingroup vixen_allocator
struct legacy_allocator : public allocator {
    virtual ~legacy_allocator() {}

    VIXEN_NODISCARD void *legacy_alloc(usize size);
    void legacy_dealloc(void *ptr);
    VIXEN_NODISCARD void *legacy_realloc(usize new_size, void *old_ptr);

protected:
    VIXEN_NODISCARD virtual void *internal_alloc(const layout &layout) override;
    virtual void internal_dealloc(const layout &layout, void *ptr) override;

    VIXEN_NODISCARD virtual void *internal_legacy_alloc(usize size) = 0;
    virtual void internal_legacy_dealloc(void *ptr) = 0;
    VIXEN_NODISCARD virtual void *internal_legacy_realloc(usize new_size, void *old_ptr) = 0;
};

// TODO: not happy about this inheritance hierarchy... I'd like something more like traits, so I
// could say "struct my_allocator : public allocator, public resettable_allocator { ... }"
/// @ingroup vixen_allocator
/// Allocators inheriting from this class are _resettable_, and may have all of their allocations
/// discarded with a single call to `reset`.
struct resettable_allocator : public allocator {
    virtual ~resettable_allocator() {}

    void reset();

protected:
    virtual void internal_reset() = 0;
};

// A non-specialized reallocation routine. It just allocates, copies, and then deallocates.
/// @ingroup vixen_allocator
VIXEN_NODISCARD inline void *general_realloc(
    allocator *alloc, const layout &old_layout, const layout &new_layout, void *old_ptr);

/// @ingroup vixen_allocator
/// @brief Allocates `sizeof(T)` bytes with alignment of `alignof(T)` using `alloc`, and returns a
/// pointer to the uninitialized data.
template <typename T>
VIXEN_NODISCARD inline T *create_uninit(allocator *alloc);

/// @ingroup vixen_allocator
/// @brief Allocates `sizeof(T)` bytes with alignment of `alignof(T)` using `alloc`, constructs T
/// in-place, and returns a pointer to the newly created object.
///
/// @note all `args` are _forwarded_ to `T`'s constructor.
template <typename T, typename... Args>
VIXEN_NODISCARD inline T *create_init(allocator *alloc, Args &&...args);

/// @ingroup vixen_allocator
/// @brief Allocates `len * sizeof(T)` bytes with alignment of `alignof(T)` using `alloc`, and
/// returns a pointer to the beginning of the uninitialized data.
template <typename T>
VIXEN_NODISCARD inline T *create_array_uninit(allocator *alloc, usize len);

/// @ingroup vixen_allocator
/// @brief Deallocates `sizeof(T)` bytes with alignment of `alignof(T)` that was previously
/// allocated with the same layout from `alloc`.
///
/// @note This is _not_ like `delete`, and does _not_ call `~T()`.
template <typename T>
inline void destroy_uninit(allocator *alloc, T *ptr);

/// @ingroup vixen_allocator
/// @brief Calls `~T()`, and deallocates `sizeof(T)` bytes with alignment of `alignof(T)` that was
/// previously allocated with the same layout from `alloc`.
template <typename T>
inline void destroy_init(allocator *alloc, T *ptr);

/// @ingroup vixen_allocator
/// @brief Dellocates `len * sizeof(T)` bytes with alignment of `alignof(T)` that was previously
/// allocated with the same layout from `alloc`.
template <typename T>
inline void destroy_array_uninit(allocator *alloc, T *ptr, usize len);

/// @ingroup vixen_allocator
template <typename H, typename... Args>
inline void dealloc_parallel(allocator *alloc, usize len, H *head, Args *...args);

/// @ingroup vixen_allocator
template <typename H, typename... Args>
inline void alloc_parallel(allocator *alloc, usize len, H **head, Args **...args);

/// @ingroup vixen_allocator
template <typename H, typename... Args>
inline void realloc_parallel(allocator *alloc, usize len, usize new_len, H **head, Args **...args);

/// @ingroup vixen_allocator
allocator *global_allocator();

/// @ingroup vixen_allocator
/// Allocator that can be used like `malloc`/`free` would be, and forwards requests to
/// `global_allocator()`.
legacy_allocator *legacy_global_allocator();

/// @ingroup vixen_allocator
/// allocator used for debug information, like storage for allocator profiles etc.
allocator *debug_allocator();

usize page_size();

} // namespace vixen::heap

/// @ingroup vixen_allocator
/// allocator is such a commonly-used type that it makes sense not to have to refer to it by
/// `heap::allocator` all the time.
using allocator = vixen::heap::allocator;

namespace vixen {

template <typename T>
using has_allocator_aware_copy_ctor_type
    = decltype(T{std::declval<allocator *>(), std::declval<const T &>()});

template <typename T, typename = void>
struct has_allocator_aware_copy_ctor : std::false_type {};

template <typename T>
struct has_allocator_aware_copy_ctor<T, std::void_t<has_allocator_aware_copy_ctor_type<T>>>
    : std::true_type {};

template <typename T>
T copy_construct_maybe_allocator_aware(allocator *alloc, const T &original) {
    if constexpr (has_allocator_aware_copy_ctor<T>::value) {
        return T(alloc, original);
    } else {
        return T(original);
    }
}

} // namespace vixen

#include "allocator/allocator.inl"