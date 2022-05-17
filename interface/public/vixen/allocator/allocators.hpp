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
    void *internalAlloc(const Layout &layout) override;
    void internalDealloc(const Layout &layout, void *ptr) override;
};

struct LegacyAdapterAllocator final : public LegacyAllocator {
    VIXEN_NODISCARD virtual void *internalLegacyAlloc(usize size) override;
    virtual void internalLegacyDealloc(void *ptr) override;
    VIXEN_NODISCARD virtual void *internalLegacyRealloc(usize newSize, void *oldPtr) override;

    LegacyAdapterAllocator(Allocator &adapted) : adapted(&adapted) {}

    Allocator *adapted;
};

// Forwards alloctions to malloc/free
struct SystemAllocator final : public LegacyAllocator {
    void *internalAlloc(const Layout &layout) override;
    void internalDealloc(const Layout &layout, void *ptr) override;

    VIXEN_NODISCARD virtual void *internalLegacyAlloc(usize size) override;
    virtual void internalLegacyDealloc(void *ptr) override;
    VIXEN_NODISCARD virtual void *internalLegacyRealloc(usize newSize, void *oldPtr) override;
};

// Blasts through memory like `ArenaAllocator` but doesn't page in more memory when
// its current block runs out.
// This allocator is a "source" and doesn't have a parent allocator.
struct LinearAllocator final : public ResettableAllocator {
    void *internalAlloc(const Layout &layout) override;
    void internalDealloc(const Layout &layout, void *ptr) override;
    void *internalRealloc(const Layout &oldLayout, const Layout &newLayout, void *ptr) override;

    void internalReset() override;

    LinearAllocator(void *block, usize len);
    ~LinearAllocator();

private:
    void *cursor;
    void *prevCursor;
    void *start;
    void *end;
};

// NOT THREADSAFE!!!
struct ArenaAllocator final : public ResettableAllocator {
    void *internalAlloc(const Layout &layout) override;
    void internalDealloc(const Layout &layout, void *ptr) override;
    void *internalRealloc(const Layout &oldLayout, const Layout &newLayout, void *ptr) override;
    void internalReset() override;

    explicit ArenaAllocator(Allocator &parent);
    ~ArenaAllocator();

    struct BlockDescriptor {
        void *current;
        void *start;
        void *end;
        void *last;
        BlockDescriptor *next;
        Layout blockLayout;
    };

private:
    void *tryAllocateIn(const Layout &layout, BlockDescriptor &block);
    void *tryAllocateInChain(const Layout &layout, BlockDescriptor &block);
    BlockDescriptor *allocateBlock(const Layout &requiredLayout);

    usize lastSize;
    BlockDescriptor *blocks;
    BlockDescriptor *currentBlock;
    Allocator *parent;
};

} // namespace vixen::heap
