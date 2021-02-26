#include "vixen/allocator/allocators.hpp"

#include <Tracy.hpp>
#include <cstddef>
#include <cstdlib>

namespace vixen::heap {

void *alloc_impl(const layout &layout) {
    if (layout.align > alignof(std::max_align_t)) {
        return std::aligned_alloc(layout.align, layout.size);
    } else {
        return std::malloc(layout.size);
    }
}

void *system_allocator::internal_alloc(const layout &layout) {
    _VIXEN_ALLOC_PROLOGUE(layout);

    void *ptr = alloc_impl(layout);
    TracyAllocN(ptr, layout.size, "system alloc");
    if (ptr == nullptr) {
        throw allocation_exception{};
    }
    return ptr;
}

void system_allocator::internal_dealloc(const layout &layout, void *ptr) {
    _VIXEN_DEALLOC_PROLOGUE(layout, ptr);

    TracyFreeN(ptr, "system alloc");
    std::free(ptr);
}

void *system_allocator::internal_legacy_alloc(usize size) {
    void *ptr = std::malloc(size);
    TracyAllocN(ptr, size, "system alloc");
    return ptr;
}

void system_allocator::internal_legacy_dealloc(void *ptr) {
    TracyFreeN(ptr, "system alloc");
    std::free(ptr);
}

void *system_allocator::internal_legacy_realloc(usize new_size, void *old_ptr) {
    return std::realloc(old_ptr, new_size);
}

} // namespace vixen::heap