#pragma once

#include "vixen/allocator/allocator.hpp"

#include "erased_storage.hpp"

namespace vixen::heap {

template <typename T>
struct PoolAllocator {
    PoolAllocator(Allocator *alloc) : parent_alloc(alloc) {}

    ~PoolAllocator() {
        Block *current = block_list_head;
        while (current != nullptr) {
            Block *next = current->next_block;
            parent_alloc->dealloc(Layout::array_of<Cell>(block_cell_count + 1), current);
            current = next;
        }
    }

    union Block {
        Block *next_block;
    };

    union Cell {
        UninitializedStorage<T> storage;
        Block *next_free_cell;
    };

    static_assert(sizeof(Cell) >= sizeof(Block));
    static_assert((sizeof(T) & (alignof(T) - 1)) == 0);

    usize block_cell_count = 1024;
    Allocator *parent_alloc;
    Block *block_list_head = nullptr;
    Block *block_list_current = nullptr;
    Cell *free_cell_head = nullptr;

    T *acquire() {
        if (unlikely(free_cell_head == nullptr)) {
            void *new_block = parent_alloc->alloc(Layout::array_of<Cell>(block_cell_count + 1));
            block_list_current->next_block = static_cast<Block *>(new_block);
            block_list_current = static_cast<Block *>(new_block);
            block_list_current->next_block = nullptr;

            usize align_mask = alignof(Cell) - 1;
            usize stride = (sizeof(Cell) + align_mask) & ~align_mask;

            Cell *current = static_cast<Cell *>(new_block) + 1;
            Cell *end = current + block_cell_count;
            Cell *next = nullptr;
            for (; current != end; ++current) {
                current->next_free_cell = next;
                next = current;
            }

            free_cell_head = current - 1;
        }

        T *ptr = std::addressof(free_cell_head->storage.get());
        free_cell_head = free_cell_head->next_free_cell;
        return ptr;
    }

    void release(T *ptr) {
        Cell *cell_ptr = reinterpret_cast<Cell *>(ptr);
        cell_ptr->next_free_cell = free_cell_head;
        free_cell_head = cell_ptr;
    }
};

} // namespace vixen::heap
