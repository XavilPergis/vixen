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
constexpr Layout Layout::array_of(usize len) {
    return {len * sizeof(T), alignof(T)};
}

constexpr Layout Layout::array(usize len) const {
    return {len * size, align};
}
constexpr Layout Layout::with_size(usize size) const {
    return {size, align};
}
constexpr Layout Layout::with_align(usize align) const {
    return {size, align};
}

constexpr Layout Layout::add_alignment(usize align) const {
    return {size, align > this->align ? this->align : align};
}

inline void *general_realloc(
    Allocator *alloc, const Layout &old_layout, const Layout &new_layout, void *old_ptr) {
    void *new_ptr = alloc->alloc(new_layout);
    if (old_ptr) {
        util::copy_nonoverlapping((u8 *)old_ptr,
            (u8 *)new_ptr,
            std::min(new_layout.size, old_layout.size));
        util::fill(DEALLOCATION_PATTERN, (u8 *)old_ptr, old_layout.size);
        if (new_layout.size > old_layout.size) {
            util::fill(ALLOCATION_PATTERN,
                (u8 *)util::offset_rawptr(new_ptr, old_layout.size),
                new_layout.size - old_layout.size);
        }
        alloc->dealloc(old_layout, old_ptr);
    }
    return new_ptr;
}

inline void *Allocator::internal_realloc(
    const Layout &old_layout, const Layout &new_layout, void *old_ptr) {
    return general_realloc(this, old_layout, new_layout, old_ptr);
}

template <typename T>
inline T *create_uninit(Allocator *alloc) {
    return (T *)alloc->alloc(Layout::of<T>());
}

template <typename T>
inline T *create_array_uninit(Allocator *alloc, usize len) {
    return (T *)alloc->alloc(Layout::array_of<T>(len));
}

template <typename T>
inline T *create_array_init(Allocator *alloc, usize len, T const &pattern) {
    T *ptr = (T *)alloc->alloc(Layout::array_of<T>(len));
    util::fill(pattern, ptr, len);
    return ptr;
}

template <typename T, typename... Args>
inline T *create_init(Allocator *alloc, Args &&...args) {
    return new (alloc->alloc(Layout::of<T>())) T{std::forward<Args>(args)...};
}

template <typename T>
inline void destroy_uninit(Allocator *alloc, T *ptr) {
    alloc->dealloc(Layout::of<T>(), ptr);
}

template <typename T>
inline void destroy_init(Allocator *alloc, T *ptr) {
    ptr->~T();
    alloc->dealloc(Layout::of<T>(), ptr);
}

template <typename T>
inline void destroy_array_uninit(Allocator *alloc, T *ptr, usize len) {
    alloc->dealloc(Layout::array_of<T>(len), ptr);
}

template <typename T>
inline void *resize_array(Allocator *alloc, T *old_ptr, usize old_len, usize new_len) {
    return alloc->realloc(Layout::array_of<T>(old_len), Layout::array_of<T>(new_len), old_ptr);
}

// --- PARALLEL DEALLOC ---

template <typename H>
inline void dealloc_parallel(Allocator *alloc, usize len, H *&head) {
    alloc->dealloc(Layout::array_of<H>(len), static_cast<void *>(head));
    head = nullptr;
}

template <typename H, typename... Args>
inline void dealloc_parallel(Allocator *alloc, usize len, H *&head, Args *&...args) {
    alloc->dealloc(Layout::array_of<H>(len), static_cast<void *>(head));
    dealloc_parallel(alloc, len, args...);
    head = nullptr;
}

// --- PARALLEL ALLOC ---

template <typename H>
inline void alloc_parallel(Allocator *alloc, usize len, H *&head) {
    head = (H *)alloc->alloc(Layout::array_of<H>(len));
}

template <typename H, typename... Args>
inline void alloc_parallel(Allocator *alloc, usize len, H *&head, Args *&...args) {
    Layout layout = Layout::array_of<H>(len);
    auto *allocated = (H *)alloc->alloc(layout);

    try {
        alloc_parallel(alloc, len, args...);
    } catch (AllocationException &ex) {
        alloc->dealloc(layout, static_cast<void *>(allocated));
        throw;
    }

    head = allocated;
}

// --- PARALLEL REALLOC ---

template <typename H>
inline void realloc_parallel(Allocator *alloc, usize old_len, usize new_len, H *&head) {
    // Since this is the lasy layer called, we don't need to worry about any of the other
    // reallocations throwing.
    head = reinterpret_cast<H *>(alloc->realloc(Layout::array_of<H>(old_len),
        Layout::array_of<H>(new_len),
        static_cast<void *>(head)));
}

template <typename H, typename... Args>
inline void realloc_parallel(
    Allocator *alloc, usize old_len, usize new_len, H *&head, Args *&...args) {
    Layout old_layout = Layout::array_of<H>(old_len);
    Layout new_layout = Layout::array_of<H>(new_len);
    H *new_head = reinterpret_cast<H *>(alloc->alloc(new_layout));

    // Split off the list and realloc the rest of the items. If one of the reallocs throw, then
    // we undo the tentative allocation and rethrow so the next layer up can do the same.
    try {
        realloc_parallel(alloc, old_len, new_len, args...);
    } catch (AllocationException &ex) {
        alloc->dealloc(new_layout, new_head);
        throw;
    }

    util::copy(head, new_head, std::min(old_len, new_len));

    // If we got here, nothing in the chain threw and we can dealloc the old and assign the new
    alloc->dealloc(old_layout, static_cast<void *>(head));
    head = new_head;
}

} // namespace vixen::heap
