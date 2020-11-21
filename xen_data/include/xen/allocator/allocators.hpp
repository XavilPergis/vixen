// This file contains various implementations of the allocator interface as defined in
// `allocator.hpp`.
#pragma once

#include "xen/allocator/allocator.hpp"

#include <xen/traits.hpp>

namespace xen::heap {
// Requests blocks of memory directly from the OS with `mmap` and releases them with `munmap`.
// This allocator is a "source" and doesn't have a parent allocator.
struct page_allocator final : public allocator {
    void *internal_alloc(const layout &layout) override;
    void internal_dealloc(const layout &layout, void *ptr) override;
};

struct legacy_adapter_allocator final : public legacy_allocator {
    XEN_NODISCARD virtual void *internal_legacy_alloc(usize size) override;
    virtual void internal_legacy_dealloc(void *ptr) override;
    XEN_NODISCARD virtual void *internal_legacy_realloc(usize new_size, void *old_ptr) override;

    legacy_adapter_allocator(allocator *adapted) : adapted(adapted) {}

    allocator *adapted;
};

// Forwards alloctions to malloc/free
struct system_allocator final : public legacy_allocator {
    void *internal_alloc(const layout &layout) override;
    void internal_dealloc(const layout &layout, void *ptr) override;

    XEN_NODISCARD virtual void *internal_legacy_alloc(usize size) override;
    virtual void internal_legacy_dealloc(void *ptr) override;
    XEN_NODISCARD virtual void *internal_legacy_realloc(usize new_size, void *old_ptr) override;
};

// Blasts through memory like `arena_allocator` but doesn't page in more memory when
// its current block runs out.
// This allocator is a "source" and doesn't have a parent allocator.
struct linear_allocator final : public resettable_allocator {
    void *internal_alloc(const layout &layout) override;
    void internal_dealloc(const layout &layout, void *ptr) override;
    void *internal_realloc(const layout &old_layout, const layout &new_layout, void *ptr) override;

    void internal_reset() override;

    linear_allocator(void *block, usize len);
    ~linear_allocator();

private:
    void *cursor;
    void *prev_cursor;
    void *start;
    void *end;
};

// NOT THREADSAFE!!!
struct arena_allocator final : public resettable_allocator {
    void *internal_alloc(const layout &layout) override;
    void internal_dealloc(const layout &layout, void *ptr) override;
    void *internal_realloc(const layout &old_layout, const layout &new_layout, void *ptr) override;
    void internal_reset() override;

    explicit arena_allocator(allocator *parent);
    ~arena_allocator();

    struct block_descriptor {
        void *current;
        void *start;
        void *end;
        void *last;
        block_descriptor *next;
        layout block_layout;
    };

private:
    void *try_allocate_in(const layout &layout, block_descriptor &block);
    void *try_allocate_in_chain(const layout &layout, block_descriptor &block);
    block_descriptor *allocate_block(const layout &required_layout);

    usize last_size;
    block_descriptor *blocks;
    block_descriptor *current_block;
    allocator *parent;
};

} // namespace xen::heap
