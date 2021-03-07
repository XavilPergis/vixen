#include "vixen/allocator/allocators.hpp"
#include "vixen/allocator/profile.hpp"

namespace vixen::heap {
void *ArenaAllocator::try_allocate_in(const Layout &layout, BlockDescriptor &block) {
    void *aligned_ptr = util::align_pointer_up(block.current, layout.align);
    void *next_ptr = (void *)((usize)aligned_ptr + layout.size);
    // Bail if the allocation won't fit in this block.
    if (next_ptr > block.end) {
        return nullptr;
    }

    block.last = block.current;
    block.current = next_ptr;
    return aligned_ptr;
}

void *ArenaAllocator::try_allocate_in_chain(const Layout &layout, BlockDescriptor &block) {
    ArenaAllocator::BlockDescriptor *current_block = &block;
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

ArenaAllocator::BlockDescriptor *ArenaAllocator::allocate_block(const Layout &required_layout) {
    BlockDescriptor *block = (BlockDescriptor *)this->parent->alloc(Layout::of<BlockDescriptor>());
    if (!block) {
        return nullptr;
    }
    usize new_size = std::max(this->last_size * 2, required_layout.size);
    void *ptr = this->parent->alloc(required_layout.with_size(new_size));
    if (!ptr) {
        this->parent->dealloc(Layout::of<BlockDescriptor>(), (void *)block);
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

void ArenaAllocator::internal_reset() {
    this->current_block = this->blocks;
    BlockDescriptor *current_block = this->blocks;
    while (current_block) {
        current_block->current = current_block->start;
        current_block = current_block->next;
    }
}

ArenaAllocator::ArenaAllocator(Allocator *alloc) : ResettableAllocator() {
    this->last_size = 32;
    this->blocks = nullptr;
    this->current_block = nullptr;
    this->parent = alloc;
}

ArenaAllocator::~ArenaAllocator() {
    reset();

    BlockDescriptor *current_block = this->blocks;
    while (current_block) {
        BlockDescriptor *next_block = current_block->next;
        this->parent->dealloc(current_block->block_layout, current_block->start);
        this->parent->dealloc(Layout::of<BlockDescriptor>(), current_block);
        current_block = next_block;
    }
}

void *ArenaAllocator::internal_realloc(
    const Layout &old_layout, const Layout &new_layout, void *old_ptr) {
    _VIXEN_REALLOC_PROLOGUE(old_layout, new_layout, old_ptr)

    // Differing alignments probably won't happen, or will be exceedingly rare, so we shouldn't
    // burden ourselves with needing to think about alignment in the rest of this method.
    bool are_alignments_same = new_layout.align == old_layout.align;
    bool fits_in_block = util::offset_rawptr(old_ptr, new_layout.size) < this->current_block->end;

    if (are_alignments_same && fits_in_block) {
        // Optimize shrinking reallocs
        if (new_layout.size < old_layout.size) {
            // Actually shrink the current block if its the last one.
            if (old_ptr == this->current_block->last) {
                this->current_block->current = util::offset_rawptr(old_ptr, new_layout.size);
                return this->current_block->current;
            }
            // Just hand back the old pointer, because shrinking a block will never trigger a
            // relocation. If we can't shrink the block because it's not current, then we can just
            // do nothing and say we shrank it.
            return old_ptr;
        }

        // Optimize last-allocation grows
        if (old_ptr == current_block->last) {
            this->current_block->current = util::offset_rawptr(old_ptr, new_layout.size);
            return old_ptr;
        }
    }

    return general_realloc(this, old_layout, new_layout, old_ptr);
}

void ArenaAllocator::internal_dealloc(const Layout &layout, void *ptr) {
    _VIXEN_DEALLOC_PROLOGUE(layout, ptr)

    // Don't grow if we can rewind the last allocation. Very good for loops that push a bunch of
    // items to a vector, since the entire thing will just grow in place.
    if (ptr == this->current_block->last) {
        this->current_block->current = this->current_block->last;
    }
}

void *ArenaAllocator::internal_alloc(const Layout &layout) {
    _VIXEN_ALLOC_PROLOGUE(layout)

    void *ptr = this->current_block == nullptr
        ? nullptr
        : try_allocate_in_chain(layout, *this->current_block);

    if (!ptr) {
        BlockDescriptor *new_block = allocate_block(layout);
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
        throw AllocationException{};
    }
    return ptr;
}

} // namespace vixen::heap
