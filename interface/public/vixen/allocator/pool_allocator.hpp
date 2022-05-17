#pragma once

#include "vixen/allocator/allocator.hpp"

#include "erased_storage.hpp"

namespace vixen::heap {

template <typename T>
struct PoolAllocator {
    PoolAllocator(Allocator &alloc) : parentAlloc(&alloc) {}

    ~PoolAllocator() {
        Block *current = blockListHead;
        while (current != nullptr) {
            Block *next = current->nextBlock;
            parentAlloc->dealloc(Layout::array_of<Cell>(block_cell_count + 1), current);
            current = next;
        }
    }

    union Block {
        Block *nextBlock;
    };

    union Cell {
        UninitializedStorage<T> storage;
        Block *nextFreeCell;
    };

    static_assert(sizeof(Cell) >= sizeof(Block));
    static_assert((sizeof(T) & (alignof(T) - 1)) == 0);

    usize block_cell_count = 1024;
    Allocator *parentAlloc;
    Block *blockListHead = nullptr;
    Block *blockListCurrent = nullptr;
    Cell *freeCellHead = nullptr;

    T *acquire() {
        if (unlikely(freeCellHead == nullptr)) {
            void *new_block = parentAlloc->alloc(Layout::array_of<Cell>(block_cell_count + 1));
            blockListCurrent->nextBlock = static_cast<Block *>(new_block);
            blockListCurrent = static_cast<Block *>(new_block);
            blockListCurrent->nextBlock = nullptr;

            usize align_mask = alignof(Cell) - 1;
            usize stride = (sizeof(Cell) + align_mask) & ~align_mask;

            Cell *current = static_cast<Cell *>(new_block) + 1;
            Cell *end = current + block_cell_count;
            Cell *next = nullptr;
            for (; current != end; ++current) {
                current->nextFreeCell = next;
                next = current;
            }

            freeCellHead = current - 1;
        }

        T *ptr = std::addressof(freeCellHead->storage.get());
        freeCellHead = freeCellHead->nextFreeCell;
        return ptr;
    }

    void release(T *ptr) {
        new (ptr) Cell{.nextFreeCell = freeCellHead};
        freeCellHead = cellPtr;
    }
};

} // namespace vixen::heap
