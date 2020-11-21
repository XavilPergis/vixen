#pragma once

#include "xen/allocator/allocator.hpp"
#include "xen/allocator/profile.hpp"

#include <algorithm>
#include <cstring>
#include <new>
#include <xen/defer.hpp>
#include <xen/types.hpp>
#include <xen/util.hpp>

namespace xen::heap {
template <typename P, typename A>
constexpr P align_up(P ptr, A align) {
    return (P)((A)ptr + (align - 1) & ~(align - 1));
}

constexpr void *void_ptr_add(void *ptr, usize bytes) {
    return (void *)((usize)ptr + bytes);
}
constexpr void *void_ptr_sub(void *ptr, usize bytes) {
    return (void *)((usize)ptr - bytes);
}

inline bool layout::operator==(const layout &rhs) const {
    return size == rhs.size && align == rhs.align;
}
inline bool layout::operator!=(const layout &rhs) const {
    return !(*this == rhs);
}

template <typename T>
constexpr layout layout::of() {
    return {sizeof(T), alignof(T)};
}

template <typename T>
constexpr layout layout::array_of(usize len) {
    return {len * sizeof(T), alignof(T)};
}

constexpr layout layout::array(usize len) const {
    return {len * size, align};
}
constexpr layout layout::with_size(usize size) const {
    return {size, align};
}
constexpr layout layout::with_align(usize align) const {
    return {size, align};
}

constexpr layout layout::add_alignment(usize align) const {
    return {size, align > this->align ? this->align : align};
}

inline void *general_realloc(
    allocator *alloc, const layout &old_layout, const layout &new_layout, void *old_ptr) {
    void *new_ptr = alloc->alloc(new_layout);
    if (old_ptr) {
        util::copy_nonoverlapping((u8 *)old_ptr,
            (u8 *)new_ptr,
            std::min(new_layout.size, old_layout.size));
        util::fill(DEALLOCATION_PATTERN, (u8 *)old_ptr, old_layout.size);
        if (new_layout.size > old_layout.size) {
            util::fill(ALLOCATION_PATTERN,
                (u8 *)void_ptr_add(new_ptr, old_layout.size),
                new_layout.size - old_layout.size);
        }
        alloc->dealloc(old_layout, old_ptr);
    }
    return new_ptr;
}

inline void *allocator::internal_realloc(
    const layout &old_layout, const layout &new_layout, void *old_ptr) {
    return general_realloc(this, old_layout, new_layout, old_ptr);
}

template <typename T>
inline T *create_uninit(allocator *alloc) {
    return (T *)alloc->alloc(layout::of<T>());
}

template <typename T>
inline T *create_array_uninit(allocator *alloc, usize len) {
    return (T *)alloc->alloc(layout::array_of<T>(len));
}

template <typename T>
inline T *create_array_init(allocator *alloc, usize len, T const &pattern) {
    T *ptr = (T *)alloc->alloc(layout::array_of<T>(len));
    util::fill(pattern, ptr, len);
    return ptr;
}

template <typename T, typename... Args>
inline T *create_init(allocator *alloc, Args &&... args) {
    return new (alloc->alloc(layout::of<T>())) T{std::forward<Args>(args)...};
}

template <typename T>
inline void destroy(allocator *alloc, T *ptr) {
    alloc->dealloc(layout::of<T>(), ptr);
}

template <typename T>
inline void destroy_array(allocator *alloc, T *ptr, usize len) {
    alloc->dealloc(layout::array_of<T>(len), ptr);
}

// --- PARALLEL DEALLOC ---

template <typename H>
inline void dealloc_parallel(allocator *alloc, usize len, H *head) {
    alloc->dealloc(layout::array_of<H>(len), (void *)head);
}

template <typename H, typename... Args>
inline void dealloc_parallel(allocator *alloc, usize len, H *head, Args *... args) {
    alloc->dealloc(layout::array_of<H>(len), (void *)head);
    dealloc_parallel(alloc, len, args...);
}

// --- PARALLEL ALLOC ---

template <typename H>
inline void alloc_parallel(allocator *alloc, usize len, H **head) {
    *head = (H *)alloc->alloc(layout::array_of<H>(len));
}

template <typename H, typename... Args>
inline void alloc_parallel(allocator *alloc, usize len, H **head, Args **... args) {
    layout layout = layout::array_of<H>(len);
    auto *allocated = (H *)alloc->alloc(layout);

    try {
        alloc_parallel(alloc, len, args...);
    } catch (allocation_exception &ex) {
        alloc->dealloc(layout, (void *)allocated);
        throw;
    }

    *head = allocated;
}

// --- PARALLEL REALLOC ---

// template <typename H>
// inline void realloc_parallel(allocator *alloc, usize old_len, usize new_len, H **head) {
//     // Since this is the lasy layer called, we don't need to worry about any of the other
//     // reallocations throwing.
//     *head
//         = alloc->realloc(layout::array_of<H>(old_len), layout::array_of<H>(new_len), (void
//         *)head);
// }

// template <typename H, typename... Args>
// inline void realloc_parallel(
//     allocator *alloc, usize old_len, usize new_len, H **head, Args **... args) {
//     layout old_layout = layout::array_of<H>(old_len);
//     layout new_layout = layout::array_of<H>(new_len);
//     H *new_head = alloc->alloc(new_layout);

//     // Split off the list and realloc the rest of the items. If one of the reallocs throw, then
//     we
//     // undo the tentative allocation and rethrow so the next layer up can do the same.
//     try {
//         realloc_parallel(alloc, old_len, new_len, args...);
//     } catch (allocation_exception &ex) {
//         alloc->dealloc(new_layout, new_head);
//         throw;
//     }

//     util::copy(*head, new_head, std::min(old_len, new_len));

//     // If we got here, nothing in the chain threw and we can dealloc the old and assign the new
//     alloc->dealloc(old_layout, (void *)*head);
//     *head = new_head;
// }

} // namespace xen::heap
