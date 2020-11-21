#include "xen/allocator/allocators.hpp"
#include "xen/allocator/profile.hpp"

#include <unistd.h>
#include <xen/assert.hpp>

namespace xen::heap {
static page_allocator g_global_allocator{};
static legacy_adapter_allocator g_global_adapter{&g_global_allocator};
static system_allocator g_debug_allocator{};

allocator *global_allocator() {
    return &g_global_allocator;
}

legacy_allocator *legacy_global_allocator() {
    return &g_global_adapter;
}

allocator *debug_allocator() {
    return &g_debug_allocator;
}

usize page_size() {
    static usize g_page_size = sysconf(_SC_PAGE_SIZE);
    return g_page_size;
}

void *allocator::alloc(const layout &layout) {
    XEN_ENGINE_ASSERT(this != nullptr,
        "Tried to allocate {}, but the allocator pointer was null.",
        layout)
    begin_transaction(id);
    void *ptr = this->internal_alloc(layout);
    record_alloc(id, layout, ptr);
    end_transaction(id);
    std::memset(ptr, ALLOCATION_PATTERN, layout.size);
    return ptr;
}

void allocator::dealloc(const layout &layout, void *ptr) {
    XEN_ENGINE_ASSERT(this != nullptr,
        "Tried to deallocate {} ({}), but the allocator pointer was null.",
        ptr,
        layout)
    std::memset(ptr, DEALLOCATION_PATTERN, layout.size);
    begin_transaction(id);
    this->internal_dealloc(layout, ptr);
    record_dealloc(id, layout, ptr);
    end_transaction(id);
}

void *allocator::realloc(const layout &old_layout, const layout &new_layout, void *old_ptr) {
    XEN_ENGINE_ASSERT(this != nullptr,
        "Tried to reallocate {} ({}) -> {}, but the allocator pointer was null.",
        old_ptr,
        old_layout,
        new_layout)

    begin_transaction(id);
    void *ptr = this->internal_realloc(old_layout, new_layout, old_ptr);
    record_realloc(id, old_layout, old_ptr, new_layout, ptr);
    end_transaction(id);

    // TODO: debug memsetting!
    return ptr;
}

void resettable_allocator::reset() {
    begin_transaction(id);
    internal_reset();
    record_reset(id);
    end_transaction(id);
}

void *legacy_allocator::internal_alloc(const layout &layout) {
    _XEN_ALLOC_PROLOGUE(layout)

    if (layout.align <= MAX_LEGACY_ALIGNMENT) {
        return internal_legacy_alloc(layout.size);
    } else {
        XEN_ASSERT(false,
            "Tried to allocate {} on legacy_allocator, which does not support alignments above {}.",
            layout,
            MAX_LEGACY_ALIGNMENT);
    }
}

void legacy_allocator::internal_dealloc(const layout &layout, void *ptr) {
    _XEN_DEALLOC_PROLOGUE(layout, ptr)

    internal_legacy_dealloc(ptr);
}

void *legacy_allocator::legacy_alloc(usize size) {
    XEN_ENGINE_ASSERT(this != nullptr,
        "Tried to legacy allocate {} bytes, but the allocator pointer was null.",
        size)
    begin_transaction(id);
    void *ptr = this->internal_legacy_alloc(size);
    record_legacy_alloc(id, size, ptr);
    end_transaction(id);
    std::memset(ptr, ALLOCATION_PATTERN, size);
    return ptr;
}

void legacy_allocator::legacy_dealloc(void *ptr) {
    XEN_ENGINE_ASSERT(this != nullptr,
        "Tried to legacy deallocate {}, but the allocator pointer was null.",
        ptr)
    begin_transaction(id);
    this->internal_legacy_dealloc(ptr);
    record_legacy_dealloc(id, ptr);
    end_transaction(id);
}

void *legacy_allocator::legacy_realloc(usize new_size, void *old_ptr) {
    XEN_ENGINE_ASSERT(this != nullptr,
        "Tried to legacy reallocate {} to new size of {} bytes, but the allocator pointer was null.",
        old_ptr,
        new_size)
    begin_transaction(id);
    void *ptr = this->internal_legacy_realloc(new_size, old_ptr);
    record_legacy_realloc(id, old_ptr, new_size, ptr);
    end_transaction(id);
    // TODO: debug memsetting!
    return ptr;
}

} // namespace xen::heap
