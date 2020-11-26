#include "vixen/allocator/allocators.hpp"

#include <cstddef>
#include <cstdlib>

namespace vixen::heap {
void *system_allocator::internal_alloc(const layout &layout) {
    _VIXEN_ALLOC_PROLOGUE(layout);

    void *ptr = std::aligned_alloc(layout.align, layout.size);
    if (ptr == nullptr) {
        throw allocation_exception{};
    }
    return ptr;
}

void system_allocator::internal_dealloc(const layout &layout, void *ptr) {
    _VIXEN_DEALLOC_PROLOGUE(layout, ptr);

    std::free(ptr);
}

void *system_allocator::internal_legacy_alloc(usize size) {
    return std::malloc(size);
}

void system_allocator::internal_legacy_dealloc(void *ptr) {
    std::free(ptr);
}

void *system_allocator::internal_legacy_realloc(usize new_size, void *old_ptr) {
    return std::realloc(old_ptr, new_size);
}

} // namespace vixen::heap