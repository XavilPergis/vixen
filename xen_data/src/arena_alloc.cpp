#include "xen/allocator/allocators.hpp"
#include "xen/allocator/profile.hpp"

namespace xen::heap {
void *arena_allocator::try_allocate_in(const layout &layout, block_descriptor &block) {
    void *aligned_ptr = align_up(block.current, layout.align);
    void *next_ptr = (void *)((usize)aligned_ptr + layout.size);
    // Bail if the allocation won't fit in this block.
    if (next_ptr > block.end) {
        return nullptr;
    }

    block.current = next_ptr;
    return aligned_ptr;
}

void *arena_allocator::try_allocate_in_chain(const layout &layout, block_descriptor &block) {
    arena_allocator::block_descriptor *current_block = &block;
    while (current_block) {
        void *allocated = try_allocate_in(layout, *current_block);
        if (allocated) {
            return allocated;
        } else {
            current_block = current_block->next;
        }
    }
    return nullptr;
}

arena_allocator::block_descriptor *arena_allocator::allocate_block(const layout &required_layout) {
    block_descriptor *block
        = (block_descriptor *)this->parent->alloc(layout::of<block_descriptor>());
    if (!block) {
        return nullptr;
    }
    usize new_size = std::max(this->last_size * 2, required_layout.size);
    void *ptr = this->parent->alloc(required_layout.with_size(new_size));
    if (!ptr) {
        this->parent->dealloc(layout::of<block_descriptor>(), (void *)block);
        return nullptr;
    }

    block->next = nullptr;
    block->start = ptr;
    block->current = block->start;
    block->end = (void *)((usize)ptr + new_size);
    block->last = ptr;
    block->block_layout = required_layout.with_size(new_size);
    return block;
}

void arena_allocator::internal_reset() {
    this->current_block = this->blocks;
    block_descriptor *current_block = this->blocks;
    while (current_block) {
        current_block->current = current_block->start;
        current_block = current_block->next;
    }
}

arena_allocator::arena_allocator(allocator *alloc) {
    this->last_size = 32;
    this->blocks = nullptr;
    this->current_block = nullptr;
    this->parent = alloc;
}

arena_allocator::~arena_allocator() {
    reset();

    block_descriptor *current_block = this->blocks;
    while (current_block) {
        block_descriptor *next_block = current_block->next;
        this->parent->dealloc(current_block->block_layout, current_block->start);
        this->parent->dealloc(layout::of<block_descriptor>(), current_block);
        current_block = next_block;
    }
}

void *arena_allocator::internal_realloc(
    const layout &old_layout, const layout &new_layout, void *old_ptr) {
    _XEN_REALLOC_PROLOGUE(old_layout, new_layout, old_ptr)

    // We could probably deal with differing alignments, but it's not really worth the extra
    // complexity...
    if (old_layout.align < new_layout.align) {
        return general_realloc(this, old_layout, new_layout, old_ptr);
    }

    // Always shrink in-place
    if (new_layout.size < old_layout.size) {
        // Actually shrink the current block if its the last one.
        if (old_ptr == this->current_block->last) {
            this->current_block->current = void_ptr_add(old_ptr, new_layout.size);
            return this->current_block->current;
        }
        // Just hand back the old pointer, because shrinking a block will never trigger a
        // relocation. If we can't shrink the block because it's not current, then we can just
        // do nothing and say we shrank it.
        return old_ptr;
    }

    // If the block we're reallocating isn't the most recent one, or if the grown block would not
    // fit, then just perform a normal realloc.
    bool is_last = old_ptr == this->current_block->last;
    bool is_block_past_end = void_ptr_add(old_ptr, new_layout.size) > this->current_block->end;
    if (!is_last || is_block_past_end) {
        return general_realloc(this, old_layout, new_layout, old_ptr);
    }

    // Grow in-place
    if (new_layout.size > old_layout.size) {
        this->current_block->current = void_ptr_add(old_ptr, new_layout.size);
        return old_ptr;
    }

    XEN_ENGINE_UNREACHABLE("arena_allocator::internal_realloc got to end of function.")
    return nullptr;
}

void arena_allocator::internal_dealloc(const layout &layout, void *ptr) {
    _XEN_DEALLOC_PROLOGUE(layout, ptr)

    // Don't grow if we can rewind the last allocation. Very good for loops that push a bunch of
    // items to a vector, since the entire thing will just grow in place.
    if (ptr == this->current_block->last) {
        this->current_block->current = this->current_block->last;
    }
}

void *arena_allocator::internal_alloc(const layout &layout) {
    _XEN_ALLOC_PROLOGUE(layout)

    void *ptr = try_allocate_in_chain(layout, *this->current_block);
    if (!ptr) {
        block_descriptor *new_block = allocate_block(layout);
        this->last_size = (usize)new_block->end - (usize)new_block->start;
        // Note that the call to `try_allocate_in_chain` changes `this->current_block`, so
        // if we get to this point, then `this->current_block` is the last block.
        if (!this->blocks) {
            this->blocks = new_block;
            this->current_block = new_block;
        } else {
            this->current_block->next = new_block;
            this->current_block = new_block;
        }

        ptr = try_allocate_in(layout, *new_block);
    }

    if (ptr == nullptr) {
        throw allocation_exception{};
    }
    return ptr;
}

} // namespace xen::heap
