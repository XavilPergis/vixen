#pragma once

#include "vixen/allocator/layout.hpp"
#include "vixen/assert.hpp"
#include "vixen/types.hpp"
#include "vixen/util.hpp"

#include <cstddef>

/// @defgroup vixen_allocator Allocators
/// @brief Pluggable dynamic memory allocators.

/**
 * @brief
 * @file
 * @ingroup vixen_allocator
 * @brief Main allocator file; defines the allocator interface, the global allocator, and various
 * allocator utilities.
 */

// These prologs normalize the behavior of allocation and deallocation funtions
// and can provide extra correctness checks, so that this code is not duplicated
// in each allocatior implementation
#define _VIXEN_ALLOC_PROLOGUE(layout)                           \
    if ((layout).size == 0) {                                   \
        return nullptr;                                         \
    }                                                           \
    VIXEN_ASSERT_EXT(vixen::util::isPowerOfTwo((layout).align), \
        "Alignment {} is not a power of 2.",                    \
        (layout).align);

#define _VIXEN_DEALLOC_PROLOGUE(layout, ptr)                    \
    if (ptr == nullptr) {                                       \
        return;                                                 \
    }                                                           \
    VIXEN_ASSERT_EXT(vixen::util::isPowerOfTwo((layout).align), \
        "Alignment {} is not a power of 2.",                    \
        (layout).align);

#define _VIXEN_REALLOC_PROLOGUE(oldLayout, newLayout, ptr)         \
    if (ptr == nullptr) {                                          \
        return internalAlloc(newLayout);                           \
    }                                                              \
    if (oldLayout == newLayout) {                                  \
        return ptr;                                                \
    }                                                              \
    VIXEN_ASSERT_EXT(vixen::util::isPowerOfTwo((oldLayout).align), \
        "Alignment {} is not a power of 2.",                       \
        (oldLayout).align);                                        \
    VIXEN_ASSERT_EXT(vixen::util::isPowerOfTwo((newLayout).align), \
        "Alignment {} is not a power of 2.",                       \
        (newLayout).align);

namespace vixen::heap {

struct AllocationException {};

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

struct AllocatorLayer;
struct Allocator;

struct LayerExecutor {
    LayerExecutor(Allocator *allocator);

    // these may be called any time.
    void *alloc(Layout const &layout);
    void dealloc(Layout const &layout, void *ptr);
    void *realloc(Layout const &oldLayout, Layout const &newLayout, void *oldPtr);

    // WARNING: these may only be called when `allocator` is a `ResettableAllocator`! If this is
    // violated, the behavior is undefined.
    void reset();

    // WARNING: these may only be called when `allocator` is a `LegacyAllocator`! If this is
    // violated, the behavior is undefined.
    void *legacyAlloc(usize size);
    void legacyDealloc(void *ptr);
    void *legacyRealloc(usize newSize, void *ptr);

    AllocatorLayer *advanceLayer();

    enum class State {
        EXECUTING_LOCAL,
        EXECUTING_GLOBAL,
    };

    Allocator *allocator;
    AllocatorLayer *currentLayer = nullptr;
    State currentState = State::EXECUTING_LOCAL;
};

struct AllocatorLayer {
    virtual ~AllocatorLayer() {}

    virtual void *alloc(LayerExecutor &exec, Layout const &layout) { return exec.alloc(layout); }
    virtual void dealloc(LayerExecutor &exec, const Layout &layout, void *ptr) {
        exec.dealloc(layout, ptr);
    }
    virtual void *realloc(
        LayerExecutor &exec, Layout const &oldLayout, Layout const &newLayout, void *oldPtr) {
        return exec.realloc(oldLayout, newLayout, oldPtr);
    }
    virtual void reset(LayerExecutor &exec) { exec.reset(); }

    virtual void *legacyAlloc(LayerExecutor &exec, usize size) { return exec.legacyAlloc(size); }
    virtual void legacyDealloc(LayerExecutor &exec, void *ptr) { exec.legacyDealloc(ptr); }
    virtual void *legacyRealloc(LayerExecutor &exec, usize newSize, void *ptr) {
        return exec.legacyRealloc(newSize, ptr);
    }

    AllocatorLayer *nextLayer;
};

struct ClearMemoryLayer : AllocatorLayer {
    virtual void *alloc(LayerExecutor &exec, Layout const &layout) override;
    virtual void dealloc(LayerExecutor &exec, const Layout &layout, void *ptr) override;
};

AllocatorLayer *getGlobalLayerHead();
void addGlobalLayer(AllocatorLayer *layer);

// ArenaAllocator alloc(globalAlloc());
// auto result = process(alloc);

// void *allocNextLayer(AllocatorLayer *layer, Allocator &alloc, const Layout &layout);

/// @ingroup vixen_allocator
/// @brief Base allocator class.
struct Allocator {
    virtual ~Allocator() {}

    VIXEN_NODISCARD void *alloc(const Layout &layout);
    void dealloc(const Layout &layout, void *ptr);
    VIXEN_NODISCARD void *realloc(const Layout &oldLayout, const Layout &newLayout, void *oldPtr);

    AllocatorLayer *layerHead = nullptr;

protected:
    VIXEN_NODISCARD virtual void *internalAlloc(const Layout &layout) = 0;
    virtual void internalDealloc(const Layout &layout, void *ptr) = 0;

    VIXEN_NODISCARD virtual void *internalRealloc(
        const Layout &oldLayout, const Layout &newLayout, void *oldPtr);

    friend struct LayerExecutor;
};

/// @ingroup vixen_allocator
struct LegacyAllocator : public Allocator {
    virtual ~LegacyAllocator() {}

    VIXEN_NODISCARD void *legacyAlloc(usize size);
    void legacyDealloc(void *ptr);
    VIXEN_NODISCARD void *legacyRealloc(usize new_size, void *oldPtr);

protected:
    VIXEN_NODISCARD virtual void *internalAlloc(const Layout &layout) override;
    virtual void internalDealloc(const Layout &layout, void *ptr) override;

    VIXEN_NODISCARD virtual void *internalLegacyAlloc(usize size) = 0;
    virtual void internalLegacyDealloc(void *ptr) = 0;
    VIXEN_NODISCARD virtual void *internalLegacyRealloc(usize new_size, void *oldPtr) = 0;

    friend struct LayerExecutor;
};

// TODO: not happy about this inheritance hierarchy... I'd like something more like traits, so I
// could say "struct my_allocator : public allocator, public ResettableAllocator { ... }"
/// @ingroup vixen_allocator
/// Allocators inheriting from this class are _resettable_, and may have all of their allocations
/// discarded with a single call to `reset`.
struct ResettableAllocator : public Allocator {
    ResettableAllocator() : Allocator() {}
    virtual ~ResettableAllocator() {}

    void reset();

protected:
    virtual void internalReset() = 0;

    friend struct LayerExecutor;
};

// A non-specialized reallocation routine. It just allocates, copies, and then deallocates.
/// @ingroup vixen_allocator
VIXEN_NODISCARD inline void *generalRealloc(
    Allocator &alloc, const Layout &oldLayout, const Layout &newLayout, void *oldPtr);

/// @ingroup vixen_allocator
/// @brief Allocates `sizeof(T)` bytes with alignment of `alignof(T)` using `alloc`, and returns a
/// pointer to the uninitialized data.
template <typename T>
VIXEN_NODISCARD inline T *createUninit(Allocator &alloc);

/// @ingroup vixen_allocator
/// @brief Allocates `sizeof(T)` bytes with alignment of `alignof(T)` using `alloc`, constructs T
/// in-place, and returns a pointer to the newly created object.
///
/// @note all `args` are _forwarded_ to `T`'s constructor.
template <typename T, typename... Args>
VIXEN_NODISCARD inline T *createInit(Allocator &alloc, Args &&...args);

/// @ingroup vixen_allocator
/// @brief Deallocates `sizeof(T)` bytes with alignment of `alignof(T)` that was previously
/// allocated with the same layout from `alloc`.
///
/// @note This is _not_ like `delete`, and does _not_ call `~T()`.
template <typename T>
inline void destroyUninit(Allocator &alloc, T *ptr);

/// @ingroup vixen_allocator
/// @brief Calls `~T()`, and deallocates `sizeof(T)` bytes with alignment of `alignof(T)` that was
/// previously allocated with the same layout from `alloc`.
template <typename T>
inline void destroyInit(Allocator &alloc, T *ptr);

/// @ingroup vixen_allocator
/// @brief Allocates `len * sizeof(T)` bytes with alignment of `alignof(T)` using `alloc`, and
/// returns a pointer to the beginning of the uninitialized data.
template <typename T>
VIXEN_NODISCARD inline T *createArrayUninit(Allocator &alloc, usize len);

/// @ingroup vixen_allocator
/// @brief Dellocates `len * sizeof(T)` bytes with alignment of `alignof(T)` that was previously
/// allocated with the same layout from `alloc`.
template <typename T>
inline void destroyArrayUninit(Allocator &alloc, T *ptr, usize len);

/// @ingroup vixen_allocator
/// @brief Resizes an allocation of size `oldLen * sizeof(T)` bytes that was previously allocated
/// with `alloc` using an alignment of `alignof(T)` to be `newLen * sizeof(T)` with the same
/// alignment and allocator.
template <typename T>
VIXEN_NODISCARD inline void *resizeArray(Allocator &alloc, T *oldPtr, usize oldLen, usize newLen);

/// @ingroup vixen_allocator
template <typename H, typename... Args>
inline void deallocParallel(Allocator &alloc, usize len, H *&head, Args *&...args);

/// @ingroup vixen_allocator
template <typename H, typename... Args>
inline void allocParallel(Allocator &alloc, usize len, H *&head, Args *&...args);

/// @ingroup vixen_allocator
template <typename H, typename... Args>
inline void reallocParallel(Allocator &alloc, usize len, usize newLen, H *&head, Args *&...args);

/// @ingroup vixen_allocator
Allocator &globalAllocator();

/// @ingroup vixen_allocator
/// Allocator that can be used like `malloc`/`free` would be, and forwards requests to
/// `global_allocator()`.
LegacyAllocator &legacyGlobalAllocator();

/// @ingroup vixen_allocator
/// allocator used for debug information, like storage for allocator profiles etc.
Allocator &debugAllocator();

usize pageSize();

struct Allocators {
    Allocators() : Allocators(globalAllocator()) {}
    Allocators(Allocator &alloc) : mPersistent(&alloc), mTemporary(&alloc) {}
    Allocators(Allocator &persistent, Allocator &temporary)
        : mPersistent(&persistent), mTemporary(&temporary) {}

    Allocator &persistent() { return *mPersistent; }
    Allocator &temporary() { return *mTemporary; }

private:
    Allocator *mPersistent;
    Allocator *mTemporary;
};

} // namespace vixen::heap

/// @ingroup vixen_allocator
/// allocator is such a commonly-used type that it makes sense not to have to refer to it by
/// `heap::allocator` all the time.
using Allocator = vixen::heap::Allocator;
using Allocators = vixen::heap::Allocators;

namespace vixen {

struct copy_tag_t {};
constexpr copy_tag_t copy_tag{};

template <typename T>
using has_allocator_aware_copy_ctor_type
    = decltype(T{copy_tag, std::declval<Allocator &>(), std::declval<const T &>()});

template <typename T, typename = void>
struct has_allocator_aware_copy_ctor : std::false_type {};

template <typename T>
struct has_allocator_aware_copy_ctor<T, std::void_t<has_allocator_aware_copy_ctor_type<T>>>
    : std::true_type {};

template <typename T>
T *copyAllocateEmplace(T *location, Allocator &alloc, T const &original) {
    if constexpr (has_allocator_aware_copy_ctor<T>::value) {
        return new (location) T{copy_tag, alloc, original};
    } else {
        (void)alloc;
        return new (location) T{original};
    }
}

template <typename T>
T copyAllocate(Allocator &alloc, const T &original) {
    if constexpr (has_allocator_aware_copy_ctor<T>::value) {
        return T(copy_tag, alloc, original);
    } else {
        (void)alloc;
        return T(original);
    }
}

// clang-format off
template <typename T>
concept IsAllocatorAwareCopyConstructible = requires(Allocator &alloc, T other) {
    { T{copy_tag, alloc, static_cast<T const &>(other)} } -> IsSame<T>;
};
// clang-format on

/**
 * meant to be overloaded by other items, ie, some sort of context that has an allocator of its own,
 * but may be verbose to write out the full expression to that allocator.
 */
constexpr Allocator &getAllocator(Allocator &alloc) {
    return alloc;
}

} // namespace vixen

#include "allocator/allocator.inl"