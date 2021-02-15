#pragma once

#include "vixen/allocator/allocator.hpp"

#include "erased_storage.hpp"

namespace vixen::heap {

template <typename T>
struct pool_allocator {
    pool_allocator(allocator *alloc) : parent_alloc(alloc) {}

    ~pool_allocator() {
        block *current = block_list_head;
        while (current != nullptr) {
            block *next = current->next_block;
            parent_alloc->dealloc(layout::array_of<cell>(block_cell_count + 1), current);
            current = next;
        }
    }

    union block {
        block *next_block;
    };

    union cell {
        uninitialized_storage<T> storage;
        block *next_free_cell;
    };

    static_assert(sizeof(cell) >= sizeof(block));
    static_assert((sizeof(T) & (alignof(T) - 1)) == 0);

    usize block_cell_count = 1024;
    allocator *parent_alloc;
    block *block_list_head = nullptr;
    block *block_list_current = nullptr;
    cell *free_cell_head = nullptr;

    T *acquire() {
        if (unlikely(free_cell_head == nullptr)) {
            void *new_block = parent_alloc->alloc(layout::array_of<cell>(block_cell_count + 1));
            block_list_current->next_block = static_cast<block *>(new_block);
            block_list_current = static_cast<block *>(new_block);
            block_list_current->next_block = nullptr;

            usize align_mask = alignof(cell) - 1;
            usize stride = (sizeof(cell) + align_mask) & ~align_mask;

            cell *current = static_cast<cell *>(new_block) + 1;
            cell *end = current + block_cell_count;
            cell *next = nullptr;
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
        cell *cell_ptr = reinterpret_cast<cell *>(ptr);
        cell_ptr->next_free_cell = free_cell_head;
        free_cell_head = cell_ptr;
    }
};

} // namespace vixen::heap
