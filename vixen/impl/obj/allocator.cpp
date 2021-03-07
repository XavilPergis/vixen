#include "vixen/allocator/allocators.hpp"
#include "vixen/allocator/profile.hpp"
#include "vixen/assert.hpp"

#include <unistd.h>

#include "Tracy.hpp"

namespace vixen::heap {

Allocator *global_allocator() {
    static SystemAllocator g_global_allocator{};
    return &g_global_allocator;
}

LegacyAllocator *legacy_global_allocator() {
    static LegacyAdapterAllocator g_global_adapter{global_allocator()};
    return &g_global_adapter;
}

Allocator *debug_allocator() {
    static SystemAllocator g_debug_allocator{};
    return &g_debug_allocator;
}

usize page_size() {
    static usize g_page_size = sysconf(_SC_PAGE_SIZE);
    return g_page_size;
}

void *Allocator::alloc(const Layout &layout) {
    begin_transaction(id);
    void *ptr = this->internal_alloc(layout);
    record_alloc(id, layout, ptr);
    end_transaction(id);
    util::fill(ALLOCATION_PATTERN, static_cast<u8 *>(ptr), layout.size);
    return ptr;
}

void Allocator::dealloc(const Layout &layout, void *ptr) {
    VIXEN_ASSERT(ptr != nullptr);
    util::fill(DEALLOCATION_PATTERN, static_cast<u8 *>(ptr), layout.size);
    begin_transaction(id);
    this->internal_dealloc(layout, ptr);
    record_dealloc(id, layout, ptr);
    end_transaction(id);
}

void *Allocator::realloc(const Layout &old_layout, const Layout &new_layout, void *old_ptr) {
    begin_transaction(id);
    void *ptr = this->internal_realloc(old_layout, new_layout, old_ptr);
    record_realloc(id, old_layout, old_ptr, new_layout, ptr);
    end_transaction(id);

    // TODO: debug memsetting!
    return ptr;
}

void ResettableAllocator::reset() {
    begin_transaction(id);
    internal_reset();
    record_reset(id);
    end_transaction(id);
}

void *LegacyAllocator::internal_alloc(const Layout &layout) {
    _VIXEN_ALLOC_PROLOGUE(layout)

    if (layout.align <= MAX_LEGACY_ALIGNMENT) {
        return internal_legacy_alloc(layout.size);
    } else {
        VIXEN_PANIC(
            "Tried to allocate {} on LegacyAllocator, which does not support alignments above {}.",
            layout,
            MAX_LEGACY_ALIGNMENT);
    }
}

void LegacyAllocator::internal_dealloc(const Layout &layout, void *ptr) {
    _VIXEN_DEALLOC_PROLOGUE(layout, ptr)

    internal_legacy_dealloc(ptr);
}

void *LegacyAllocator::legacy_alloc(usize size) {
    VIXEN_ASSERT_EXT(this != nullptr,
        "Tried to legacy allocate {} bytes, but the allocator pointer was null.",
        size);
    begin_transaction(id);
    void *ptr = this->internal_legacy_alloc(size);
    record_legacy_alloc(id, size, ptr);
    end_transaction(id);
    util::fill(ALLOCATION_PATTERN, static_cast<u8 *>(ptr), size);
    return ptr;
}

void LegacyAllocator::legacy_dealloc(void *ptr) {
    VIXEN_ASSERT_EXT(this != nullptr,
        "Tried to legacy deallocate {}, but the allocator pointer was null.",
        ptr);
    begin_transaction(id);
    this->internal_legacy_dealloc(ptr);
    record_legacy_dealloc(id, ptr);
    end_transaction(id);
}

void *LegacyAllocator::legacy_realloc(usize new_size, void *old_ptr) {
    VIXEN_ASSERT_EXT(this != nullptr,
        "Tried to legacy reallocate {} to new size of {} bytes, but the allocator pointer was null.",
        old_ptr,
        new_size);
    begin_transaction(id);
    void *ptr = this->internal_legacy_realloc(new_size, old_ptr);
    record_legacy_realloc(id, old_ptr, new_size, ptr);
    end_transaction(id);
    // TODO: debug memsetting!
    return ptr;
}

} // namespace vixen::heap
