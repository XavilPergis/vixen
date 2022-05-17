#include "vixen/allocator/allocators.hpp"

#if defined(VIXEN_PLATFORM_LINUX)
#include <sys/mman.h>
#elif defined(VIXEN_PLATFORM_WINDOWS)
#include <memoryapi.h>
#include <windows.h>
#include <winnt.h>
#endif

namespace vixen::heap {
static usize allocationSize(const Layout &layout) {
    // If this layout's alignment is less the the page alignment, then we know that the alignment
    // requirements for layout will be satisfied in whatever is returned by mmap. However, if
    // layout's alignment is larger than the page alignment, we need an extra alignment's worth
    // of padding minus one page to play with to ensure that out allocation will be properly
    // aligned.
    usize sizeWithAlign = layout.align > pageSize() ? layout.size + layout.align : layout.size;
    usize allocationSize = util::alignUp(sizeWithAlign, pageSize());

    return allocationSize;
}

#if defined(VIXEN_PLATFORM_LINUX)
void *PageAllocator::internalAlloc(const Layout &layout) {
    _VIXEN_ALLOC_PROLOGUE(layout);

    usize size = allocationSize(layout);
    void *basePtr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (basePtr == MAP_FAILED) {
        throw AllocationException{};
    }
    void *alignedPtr = util::alignPointerUp(basePtr, layout.align);

    // Large alignments will often leave a bunch of padding before the usable allocation, so we
    // just unmap that here so we don't have to keep track of anything for dealloc.
    if (basePtr != alignedPtr) {
        munmap(basePtr, (usize)alignedPtr - (usize)basePtr);
    }

    return alignedPtr;
}

void PageAllocator::internalDealloc(const Layout &layout, void *ptr) {
    _VIXEN_DEALLOC_PROLOGUE(layout, ptr)

    // Note that munmap should never fail here because we will never split a mapped region, only
    // unmap the entire range.
    munmap(ptr, allocationSize(layout));
}
#endif

#if defined(VIXEN_PLATFORM_WINDOWS)

void *PageAllocator::internalAlloc(const Layout &layout) {
    _VIXEN_ALLOC_PROLOGUE(layout);

    usize size = allocationSize(layout);
    auto basePtr = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_NOACCESS);
    if (basePtr == nullptr) {
        throw AllocationException{};
    }
    void *alignedPtr = util::alignPointerUp(basePtr, layout.align);

    return alignedPtr;
}

void PageAllocator::internalDealloc(const Layout &layout, void *ptr) {
    _VIXEN_DEALLOC_PROLOGUE(layout, ptr)

    VirtualFree(ptr, 0, MEM_RELEASE);
}
#endif

} // namespace vixen::heap
