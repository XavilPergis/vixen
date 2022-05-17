#include "vixen/allocator/allocators.hpp"
#include "vixen/allocator/profile.hpp"

namespace vixen::heap {
void *ArenaAllocator::tryAllocateIn(const Layout &layout, BlockDescriptor &block) {
    void *alignedPtr = util::alignPointerUp(block.current, layout.align);
    void *nextPtr = util::offsetRaw(alignedPtr, layout.size);

    // Bail if the allocation won't fit in this block.
    if (nextPtr > block.end) {
        return nullptr;
    }

    block.last = block.current;
    block.current = nextPtr;
    return alignedPtr;
}

void *ArenaAllocator::tryAllocateInChain(const Layout &layout, BlockDescriptor &block) {
    ArenaAllocator::BlockDescriptor *currentBlock = &block;
    while (currentBlock) {
        void *allocated = tryAllocateIn(layout, *currentBlock);
        if (allocated) {
            return allocated;
        } else {
            currentBlock = currentBlock->next;
        }
    }
    return nullptr;
}

ArenaAllocator::BlockDescriptor *ArenaAllocator::allocateBlock(const Layout &requiredLayout) {
    auto *block = createInit<BlockDescriptor>(*parent);
    if (!block) return nullptr;

    usize newSize = std::max(lastSize * 2, requiredLayout.size);
    void *ptr = parent->alloc(requiredLayout.withSize(newSize));
    if (!ptr) {
        destroyInit(*parent, block);
        return nullptr;
    }

    block->next = nullptr;
    block->start = ptr;
    block->current = block->start;
    block->end = util::offsetRaw(ptr, newSize);
    block->last = ptr;
    block->blockLayout = requiredLayout.withSize(newSize);
    return block;
}

void ArenaAllocator::internalReset() {
    currentBlock = blocks;
    BlockDescriptor *currentBlock = blocks;
    while (currentBlock) {
        currentBlock->current = currentBlock->start;
        currentBlock = currentBlock->next;
    }
}

ArenaAllocator::ArenaAllocator(Allocator &alloc) : ResettableAllocator() {
    lastSize = 32;
    blocks = nullptr;
    currentBlock = nullptr;
    parent = &alloc;
}

ArenaAllocator::~ArenaAllocator() {
    reset();

    BlockDescriptor *currentBlock = blocks;
    while (currentBlock) {
        BlockDescriptor *next_block = currentBlock->next;
        parent->dealloc(currentBlock->blockLayout, currentBlock->start);
        parent->dealloc(Layout::of<BlockDescriptor>(), currentBlock);
        currentBlock = next_block;
    }
}

void *ArenaAllocator::internalRealloc(
    const Layout &oldLayout, const Layout &newLayout, void *oldPtr) {
    _VIXEN_REALLOC_PROLOGUE(oldLayout, newLayout, oldPtr)

    // Differing alignments probably won't happen, or will be exceedingly rare, so we shouldn't
    // burden ourselves with needing to think about alignment in the rest of this method.
    bool areAlignmentsSame = newLayout.align == oldLayout.align;
    bool fitsInBlock = util::offsetRaw(oldPtr, newLayout.size) < currentBlock->end;

    if (areAlignmentsSame && fitsInBlock) {
        // Optimize shrinking reallocs
        if (newLayout.size < oldLayout.size) {
            // Actually shrink the current block if its the last one.
            if (oldPtr == currentBlock->last) {
                currentBlock->current = util::offsetRaw(oldPtr, newLayout.size);
                return currentBlock->current;
            }
            // Just hand back the old pointer, because shrinking a block will never trigger a
            // relocation. If we can't shrink the block because it's not current, then we can just
            // do nothing and say we shrank it.
            return oldPtr;
        }

        // Optimize last-allocation grows
        if (oldPtr == currentBlock->last) {
            currentBlock->current = util::offsetRaw(oldPtr, newLayout.size);
            return oldPtr;
        }
    }

    return generalRealloc(*this, oldLayout, newLayout, oldPtr);
}

void ArenaAllocator::internalDealloc(const Layout &layout, void *ptr) {
    _VIXEN_DEALLOC_PROLOGUE(layout, ptr)

    // Don't grow if we can rewind the last allocation. Very good for loops that push a bunch of
    // items to a vector, since the entire thing will just grow in place.
    if (ptr == currentBlock->last) {
        currentBlock->current = currentBlock->last;
    }
}

void *ArenaAllocator::internalAlloc(const Layout &layout) {
    _VIXEN_ALLOC_PROLOGUE(layout)

    void *ptr = currentBlock == nullptr ? nullptr : tryAllocateInChain(layout, *currentBlock);

    if (!ptr) {
        BlockDescriptor *newBlock = allocateBlock(layout);
        lastSize = (usize)newBlock->end - (usize)newBlock->start;
        // Note that the call to `tryAllocateInChain` changes `currentBlock`, so
        // if we get to this point, then `currentBlock` is the last block.
        if (!blocks) {
            blocks = newBlock;
            currentBlock = newBlock;
        } else {
            currentBlock->next = newBlock;
            currentBlock = newBlock;
        }

        ptr = tryAllocateIn(layout, *newBlock);
    }

    if (ptr == nullptr) {
        throw AllocationException{};
    }
    return ptr;
}

} // namespace vixen::heap
