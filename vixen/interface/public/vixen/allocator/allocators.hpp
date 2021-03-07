// This file contains various implementations of the allocator interface as defined in
// `allocator.hpp`.
#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/traits.hpp"

#include "erased_storage.hpp"

/// @file
/// @ingroup vixen_allocator

namespace vixen::heap {
// Requests blocks of memory directly from the OS with `mmap` and releases them with `munmap`.
// This allocator is a "source" and doesn't have a parent allocator.
struct PageAllocator final : public Allocator {
    void *internal_alloc(const Layout &layout) override;
    void internal_dealloc(const Layout &layout, void *ptr) override;
};

struct LegacyAdapterAllocator final : public LegacyAllocator {
    VIXEN_NODISCARD virtual void *internal_legacy_alloc(usize size) override;
    virtual void internal_legacy_dealloc(void *ptr) override;
    VIXEN_NODISCARD virtual void *internal_legacy_realloc(usize new_size, void *old_ptr) override;

    LegacyAdapterAllocator(Allocator *adapted) : adapted(adapted) {}

    Allocator *adapted;
};

// Forwards alloctions to malloc/free
struct SystemAllocator final : public LegacyAllocator {
    void *internal_alloc(const Layout &layout) override;
    void internal_dealloc(const Layout &layout, void *ptr) override;

    VIXEN_NODISCARD virtual void *internal_legacy_alloc(usize size) override;
    virtual void internal_legacy_dealloc(void *ptr) override;
    VIXEN_NODISCARD virtual void *internal_legacy_realloc(usize new_size, void *old_ptr) override;
};

// Blasts through memory like `ArenaAllocator` but doesn't page in more memory when
// its current block runs out.
// This allocator is a "source" and doesn't have a parent allocator.
struct LinearAllocator final : public ResettableAllocator {
    void *internal_alloc(const Layout &layout) override;
    void internal_dealloc(const Layout &layout, void *ptr) override;
    void *internal_realloc(const Layout &old_layout, const Layout &new_layout, void *ptr) override;

    void internal_reset() override;

    LinearAllocator(void *block, usize len);
    ~LinearAllocator();

private:
    void *cursor;
    void *prev_cursor;
    void *start;
    void *end;
};

// NOT THREADSAFE!!!
struct ArenaAllocator final : public ResettableAllocator {
    void *internal_alloc(const Layout &layout) override;
    void internal_dealloc(const Layout &layout, void *ptr) override;
    void *internal_realloc(const Layout &old_layout, const Layout &new_layout, void *ptr) override;
    void internal_reset() override;

    explicit ArenaAllocator(Allocator *parent);
    ~ArenaAllocator();

    struct BlockDescriptor {
        void *current;
        void *start;
        void *end;
        void *last;
        BlockDescriptor *next;
        Layout block_layout;
    };

private:
    void *try_allocate_in(const Layout &layout, BlockDescriptor &block);
    void *try_allocate_in_chain(const Layout &layout, BlockDescriptor &block);
    BlockDescriptor *allocate_block(const Layout &required_layout);

    usize last_size;
    BlockDescriptor *blocks;
    BlockDescriptor *current_block;
    Allocator *parent;
};

} // namespace vixen::heap
