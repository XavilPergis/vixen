#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/allocator/profile.hpp"
#include "vixen/defer.hpp"
#include "vixen/types.hpp"
#include "vixen/util.hpp"

#include <algorithm>
#include <cstring>
#include <new>

namespace vixen::heap {

inline bool Layout::operator==(const Layout &rhs) const {
    return size == rhs.size && align == rhs.align;
}
inline bool Layout::operator!=(const Layout &rhs) const {
    return !(*this == rhs);
}

template <typename T>
constexpr Layout Layout::of() {
    return {sizeof(T), alignof(T)};
}

template <typename T>
constexpr Layout Layout::arrayOf(usize len) {
    return {len * sizeof(T), alignof(T)};
}

constexpr Layout Layout::array(usize len) const {
    return {len * size, align};
}
constexpr Layout Layout::withSize(usize size) const {
    return {size, align};
}
constexpr Layout Layout::withAlign(usize align) const {
    return {size, align};
}

constexpr Layout Layout::addAlignment(usize align) const {
    return {size, align > this->align ? this->align : align};
}

inline void *generalRealloc(
    Allocator &alloc, const Layout &oldLayout, const Layout &newLayout, void *oldPtr) {
    void *newPtr = alloc.alloc(newLayout);
    if (oldPtr) {
        util::arrayCopyNonoverlappingRaw(oldPtr, newPtr, std::min(newLayout.size, oldLayout.size));

        // since 'oldPtr' has no object in it by the point we get here, writing anything to that
        // memory is likely UB, so we need to
        for (usize i = 0; i < oldLayout.size; i++) {
            auto *addr = util::offsetRaw(oldPtr, i);
            *std::launder(new (addr) char) = DEALLOCATION_PATTERN;
        }

        if (newLayout.size > oldLayout.size) {
            // see note above
            for (usize i = 0; i < newLayout.size - oldLayout.size; i++) {
                auto *addr = util::offsetRaw(newPtr, oldLayout.size + i);
                *std::launder(new (addr) char) = ALLOCATION_PATTERN;
            }
        }

        alloc.dealloc(oldLayout, oldPtr);
    }
    return newPtr;
}

inline void *Allocator::internalRealloc(
    const Layout &oldLayout, const Layout &newLayout, void *oldPtr) {
    return generalRealloc(*this, oldLayout, newLayout, oldPtr);
}

template <typename T>
inline T *createUninit(Allocator &alloc) {
    return (T *)alloc.alloc(Layout::of<T>());
}

template <typename T>
inline T *createArrayUninit(Allocator &alloc, usize len) {
    return (T *)alloc.alloc(Layout::arrayOf<T>(len));
}

inline char *createByteArrayUninit(Allocator &alloc, usize bytes) {
    return (char *)alloc.alloc(Layout::arrayOf<char>(bytes));
}

template <typename T>
inline T *createArrayInit(Allocator &alloc, usize len, T const &pattern) {
    T *storage = createArrayUninit<T>(alloc, len);
    for (usize i = 0; i < len; ++i) {
        // placement-new needed to properly initialize object storage.
        new (&storage[i]) T{pattern};
    }
    return storage;
}

template <typename T, typename... Args>
inline T *createInit(Allocator &alloc, Args &&...args) {
    return new (alloc.alloc(Layout::of<T>())) T{std::forward<Args>(args)...};
}

template <typename T>
inline void destroyUninit(Allocator &alloc, T *ptr) {
    alloc.dealloc(Layout::of<T>(), ptr);
}

template <typename T>
inline void destroyInit(Allocator &alloc, T *ptr) {
    ptr->~T();
    alloc.dealloc(Layout::of<T>(), ptr);
}

template <typename T>
inline void destroyArrayUninit(Allocator &alloc, T *ptr, usize len) {
    alloc.dealloc(Layout::arrayOf<T>(len), ptr);
}

inline void destroyByteArrayUninit(Allocator &alloc, char *ptr, usize len) {
    alloc.dealloc(Layout::arrayOf<char>(len), ptr);
}

template <typename T>
inline void *resizeArray(Allocator &alloc, T *oldPtr, usize oldLen, usize newLen) {
    return alloc.realloc(Layout::arrayOf<T>(oldLen), Layout::arrayOf<T>(newLen), oldPtr);
}

// --- PARALLEL DEALLOC ---

template <typename H>
inline void deallocParallel(Allocator &alloc, usize len, H *&head) {
    alloc.dealloc(Layout::arrayOf<H>(len), static_cast<void *>(head));
    head = nullptr;
}

template <typename H, typename... Args>
inline void deallocParallel(Allocator &alloc, usize len, H *&head, Args *&...args) {
    alloc.dealloc(Layout::arrayOf<H>(len), static_cast<void *>(head));
    deallocParallel(alloc, len, args...);
    head = nullptr;
}

// --- PARALLEL ALLOC ---

template <typename H>
inline void allocParallel(Allocator &alloc, usize len, H *&head) {
    head = (H *)alloc.alloc(Layout::arrayOf<H>(len));
}

template <typename H, typename... Args>
inline void allocParallel(Allocator &alloc, usize len, H *&head, Args *&...args) {
    Layout layout = Layout::arrayOf<H>(len);
    auto *allocated = (H *)alloc.alloc(layout);

    try {
        allocParallel(alloc, len, args...);
    } catch (AllocationException &ex) {
        alloc.dealloc(layout, static_cast<void *>(allocated));
        throw;
    }

    head = allocated;
}

// --- PARALLEL REALLOC ---

template <typename H>
inline void reallocParallel(Allocator &alloc, usize oldLen, usize newLen, H *&head) {
    // Since this is the lasy layer called, we don't need to worry about any of the other
    // reallocations throwing.
    head = reinterpret_cast<H *>(alloc.realloc(Layout::arrayOf<H>(oldLen),
        Layout::arrayOf<H>(newLen),
        static_cast<void *>(head)));
}

template <typename H, typename... Args>
inline void reallocParallel(
    Allocator &alloc, usize oldLen, usize newLen, H *&head, Args *&...args) {
    Layout oldLayout = Layout::arrayOf<H>(oldLen);
    Layout newLayout = Layout::arrayOf<H>(newLen);
    H *newHead = reinterpret_cast<H *>(alloc.alloc(newLayout));

    // Split off the list and realloc the rest of the items. If one of the reallocs throw, then
    // we undo the tentative allocation and rethrow so the next layer up can do the same.
    try {
        reallocParallel(alloc, oldLen, newLen, args...);
    } catch (AllocationException &ex) {
        alloc.dealloc(newLayout, newHead);
        throw;
    }

    util::arrayCopy(head, newHead, std::min(oldLen, newLen));

    // If we got here, nothing in the chain threw and we can dealloc the old and assign the new
    alloc.dealloc(oldLayout, static_cast<void *>(head));
    head = newHead;
}

} // namespace vixen::heap
