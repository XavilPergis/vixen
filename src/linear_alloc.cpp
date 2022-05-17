#include "vixen/allocator/allocators.hpp"

namespace vixen::heap {
LinearAllocator::LinearAllocator(void *block, usize len) {
    this->start = block;
    this->end = util::offsetRaw(block, len);
    this->cursor = block;
    this->prevCursor = block;
}

void LinearAllocator::internalReset() {
    this->cursor = this->start;
    this->prevCursor = this->start;
}

// NOTE: look at the implemntation of `ArenaAlloc::internalRealloc` for explanation!
void *LinearAllocator::internalRealloc(
    const Layout &old_layout, const Layout &new_layout, void *old_ptr) {
    _VIXEN_REALLOC_PROLOGUE(old_layout, new_layout, old_ptr)

    if (old_layout.align < new_layout.align) {
        return generalRealloc(*this, old_layout, new_layout, old_ptr);
    }

    if (new_layout.size < old_layout.size) {
        if (old_ptr == this->prevCursor) {
            this->cursor = util::offsetRaw(old_ptr, new_layout.size);
            return this->cursor;
        }
        return old_ptr;
    }

    if (old_ptr != this->prevCursor || util::offsetRaw(old_ptr, new_layout.size) > this->end) {
        return generalRealloc(*this, old_layout, new_layout, old_ptr);
    }

    // Grow in-place
    if (new_layout.size > old_layout.size) {
        this->cursor = util::offsetRaw(old_ptr, new_layout.size);
        return old_ptr;
    }

    VIXEN_UNREACHABLE("LinearAllocator::internalRealloc got to end of function.");
    return nullptr;
}

void *LinearAllocator::internalAlloc(const Layout &layout) {
    _VIXEN_ALLOC_PROLOGUE(layout)

    void *base = util::alignPointerUp(this->cursor, layout.align);
    void *next = util::offsetRaw(base, layout.size);
    if (next > this->end) {
        throw AllocationException{};
    }
    this->cursor = next;
    return base;
}

void LinearAllocator::internalDealloc(const Layout &layout, void *ptr) {
    _VIXEN_DEALLOC_PROLOGUE(layout, ptr)

    if (this->prevCursor == ptr) {
        this->cursor = this->prevCursor;
    }
}

} // namespace vixen::heap
