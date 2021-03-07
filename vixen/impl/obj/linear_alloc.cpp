#include "vixen/allocator/allocators.hpp"

namespace vixen::heap {
LinearAllocator::LinearAllocator(void *block, usize len) {
    this->start = block;
    this->end = util::offset_rawptr(block, len);
    this->cursor = block;
    this->prev_cursor = block;
}

void LinearAllocator::internal_reset() {
    this->cursor = this->start;
    this->prev_cursor = this->start;
}

// NOTE: look at the implemntation of `ArenaAlloc::internal_realloc` for explanation!
void *LinearAllocator::internal_realloc(
    const Layout &old_layout, const Layout &new_layout, void *old_ptr) {
    _VIXEN_REALLOC_PROLOGUE(old_layout, new_layout, old_ptr)

    if (old_layout.align < new_layout.align) {
        return general_realloc(this, old_layout, new_layout, old_ptr);
    }

    if (new_layout.size < old_layout.size) {
        if (old_ptr == this->prev_cursor) {
            this->cursor = util::offset_rawptr(old_ptr, new_layout.size);
            return this->cursor;
        }
        return old_ptr;
    }

    if (old_ptr != this->prev_cursor || util::offset_rawptr(old_ptr, new_layout.size) > this->end) {
        return general_realloc(this, old_layout, new_layout, old_ptr);
    }

    // Grow in-place
    if (new_layout.size > old_layout.size) {
        this->cursor = util::offset_rawptr(old_ptr, new_layout.size);
        return old_ptr;
    }

    VIXEN_UNREACHABLE("LinearAllocator::internal_realloc got to end of function.");
    return nullptr;
}

void *LinearAllocator::internal_alloc(const Layout &layout) {
    _VIXEN_ALLOC_PROLOGUE(layout)

    void *base = util::align_pointer_up(this->cursor, layout.align);
    void *next = util::offset_rawptr(base, layout.size);
    if (next > this->end) {
        throw AllocationException{};
    }
    this->cursor = next;
    return base;
}

void LinearAllocator::internal_dealloc(const Layout &layout, void *ptr) {
    _VIXEN_DEALLOC_PROLOGUE(layout, ptr)

    if (this->prev_cursor == ptr) {
        this->cursor = this->prev_cursor;
    }
}

} // namespace vixen::heap
