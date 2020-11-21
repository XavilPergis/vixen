#pragma once

#include "xen/allocator/layout.hpp"
#include "xen/allocator/profile.hpp"

#include <cstddef>
#include <xen/assert.hpp>
#include <xen/types.hpp>
#include <xen/util.hpp>

// These prologs normalize the behavior of allocation and deallocation funtions
// and can provide extra correctness checks, so that this code is not duplicated
// in each allocatior implementation
#define _XEN_ALLOC_PROLOGUE(layout)                               \
    if ((layout).size == 0) {                                     \
        return nullptr;                                           \
    }                                                             \
    XEN_ENGINE_ASSERT(xen::util::is_power_of_two((layout).align), \
        "Alignment {} is not a power of 2.",                      \
        (layout).align)

#define _XEN_DEALLOC_PROLOGUE(layout, ptr)                        \
    if (ptr == nullptr) {                                         \
        return;                                                   \
    }                                                             \
    XEN_ENGINE_ASSERT(xen::util::is_power_of_two((layout).align), \
        "Alignment {} is not a power of 2.",                      \
        (layout).align)

#define _XEN_REALLOC_PROLOGUE(old_layout, new_layout, ptr)            \
    if (ptr == nullptr) {                                             \
        return internal_alloc(new_layout);                            \
    }                                                                 \
    if (old_layout == new_layout) {                                   \
        return ptr;                                                   \
    }                                                                 \
    XEN_ENGINE_ASSERT(xen::util::is_power_of_two((old_layout).align), \
        "Alignment {} is not a power of 2.",                          \
        (old_layout).align)                                           \
    XEN_ENGINE_ASSERT(xen::util::is_power_of_two((new_layout).align), \
        "Alignment {} is not a power of 2.",                          \
        (new_layout).align)

namespace xen::heap {
template <typename P, typename A>
constexpr P align_up(P ptr, A align);

constexpr void *void_ptr_add(void *ptr, usize bytes);
constexpr void *void_ptr_sub(void *ptr, usize bytes);

struct allocation_exception {};

constexpr u8 ALLOCATION_PATTERN = 0xae;
constexpr u8 DEALLOCATION_PATTERN = 0xfe;
constexpr usize MAX_LEGACY_ALIGNMENT = alignof(std::max_align_t);

/*
layout:
An instance of layout (named "l") must satisfy:
    - l.align must be a power of two unless l.size is 0, in which case l.align may also be 0.

alloc:
For an allocator request R:
    - A is the allocator that the allocation request was submitted to.
    - P is a pointer returned by R if R is successful.
    - L is the layout associated with the allocation request.

    - Arbitrary use must be allowed for any address in [P, P+L.size)
    - P must be aligned to L.align.
    - If R cannot be fulfilled, then an instance of AllocationException must be thrown.

dealloc:
For a deallocation request R:
    - A is the allocator that the deallocation request was submitted to.
    - P is the pointer associated with the deallocation request.
    - L is the layout associated with the deallocation request.

    - P must have been created with an allocation request to A.
    - L must be equal to the layout that P was created with.
    - R must not throw.
*/
struct allocator {
    virtual ~allocator() {}

    XEN_NODISCARD void *alloc(const layout &layout);
    void dealloc(const layout &layout, void *ptr);
    XEN_NODISCARD void *realloc(const layout &old_layout, const layout &new_layout, void *old_ptr);

    allocator_id id = NOT_TRACKED_ID;

protected:
    XEN_NODISCARD virtual void *internal_alloc(const layout &layout) = 0;
    virtual void internal_dealloc(const layout &layout, void *ptr) = 0;

    XEN_NODISCARD virtual void *internal_realloc(
        const layout &old_layout, const layout &new_layout, void *old_ptr);
};

struct legacy_allocator : public allocator {
    virtual ~legacy_allocator() {}

    XEN_NODISCARD void *legacy_alloc(usize size);
    void legacy_dealloc(void *ptr);
    XEN_NODISCARD void *legacy_realloc(usize new_size, void *old_ptr);

protected:
    XEN_NODISCARD virtual void *internal_alloc(const layout &layout) override;
    virtual void internal_dealloc(const layout &layout, void *ptr) override;

    XEN_NODISCARD virtual void *internal_legacy_alloc(usize size) = 0;
    virtual void internal_legacy_dealloc(void *ptr) = 0;
    XEN_NODISCARD virtual void *internal_legacy_realloc(usize new_size, void *old_ptr) = 0;
};

// TODO: not happy about this inheritance hierarchy... I'd like something more like traits, so I
// could say "struct my_allocator : public allocator, public resettable_allocator { ... }"
struct resettable_allocator : public allocator {
    virtual ~resettable_allocator() {}

    void reset();

protected:
    virtual void internal_reset() = 0;
};

// A non-specialized reallocation routine. It just allocates, copies, and then deallocates.
XEN_NODISCARD inline void *general_realloc(
    allocator *alloc, const layout &old_layout, const layout &new_layout, void *old_ptr);

// Allocate space for a `T` but without initializing it.
template <typename T>
XEN_NODISCARD inline T *create_uninit(allocator *alloc);

// allocator space for a `T` and construct it with the passed-in arguments.
template <typename T, typename... Args>
XEN_NODISCARD inline T *create_init(allocator *alloc, Args &&... args);

// allocator space for a `T` and construct it with the passed-in arguments.
template <typename T>
XEN_NODISCARD inline T *create_array_uninit(allocator *alloc, usize len);

template <typename T>
inline void destroy(allocator *alloc, T *ptr);

template <typename T>
inline void destroy_array(allocator *alloc, T *ptr, usize len);

template <typename H, typename... Args>
inline void dealloc_parallel(allocator *alloc, usize len, H *head, Args *... args);
template <typename H, typename... Args>
inline void alloc_parallel(allocator *alloc, usize len, H **head, Args **... args);
template <typename H, typename... Args>
inline void realloc_parallel(allocator *alloc, usize len, usize new_len, H **head, Args **... args);

allocator *global_allocator();

/// allocator that can be used like `malloc`/`free` would be, and forwardws requests to
/// `global_allocator()`.
legacy_allocator *legacy_global_allocator();

/// allocator used for debug information, like storage for allocator profiles etc.
allocator *debug_allocator();

usize page_size();

} // namespace xen::heap

// allocator is such a commonly-used type that it makes sense not to have to refer to it by
// `heap::allocator` all the time.
using allocator = xen::heap::allocator;

namespace xen {

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

} // namespace xen

#include "inl/allocator/allocator.inl"