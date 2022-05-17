#include "vixen/allocator/allocators.hpp"
#include "vixen/prof.hpp"

#include <cstddef>
#include <cstdlib>

namespace vixen::heap {

void *allocAligned(const Layout &layout) {
#if defined(VIXEN_PLATFORM_LINUX)
    return std::aligned_alloc(layout.align, layout.size);
#elif defined(VIXEN_PLATFORM_WINDOWS)
    return _aligned_malloc(layout.size, layout.align);
#endif
}

void freeAligned(void *ptr, const Layout &layout) {
#if defined(VIXEN_PLATFORM_LINUX)
    std::free(ptr);
#elif defined(VIXEN_PLATFORM_WINDOWS)
    _aligned_free(ptr);
#endif
}

void *SystemAllocator::internalAlloc(const Layout &layout) {
    _VIXEN_ALLOC_PROLOGUE(layout);

    void *ptr = allocAligned(layout);
    if (ptr == nullptr) {
        throw AllocationException{};
    }
    VIXEN_PROF_ALLOC_EVENT_NAMED(ptr, layout.size, "system alloc");
    return ptr;
}

void SystemAllocator::internalDealloc(const Layout &layout, void *ptr) {
    _VIXEN_DEALLOC_PROLOGUE(layout, ptr);

    freeAligned(ptr, layout);
    VIXEN_PROF_DEALLOC_EVENT_NAMED(ptr, "system alloc");
}

void *SystemAllocator::internalLegacyAlloc(usize size) {
    void *ptr = std::malloc(size);
    VIXEN_PROF_ALLOC_EVENT_NAMED(ptr, size, "system alloc");
    return ptr;
}

void SystemAllocator::internalLegacyDealloc(void *ptr) {
    VIXEN_PROF_DEALLOC_EVENT_NAMED(ptr, "system alloc");
    std::free(ptr);
}

void *SystemAllocator::internalLegacyRealloc(usize new_size, void *old_ptr) {
    return std::realloc(old_ptr, new_size);
}

} // namespace vixen::heap