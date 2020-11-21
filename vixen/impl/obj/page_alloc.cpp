#include "vixen/allocator/allocators.hpp"

#include <sys/mman.h>

namespace vixen::heap {
static usize allocation_size(const layout &layout) {
    // If this layout's alignment is less the the page alignment, then we know that the alignment
    // requirements for layout will be satisfied in whatever is returned by mmap. However, if
    // layout's alignment is larger than the page alignment, we need an extra alignment's worth
    // of padding minus one page to play with to ensure that out allocation will be properly
    // aligned.
    usize size_with_align = layout.align > page_size() ? layout.size + layout.align : layout.size;
    usize allocation_size = align_up(size_with_align, page_size());

    return allocation_size;
}

void *page_allocator::internal_alloc(const layout &layout) {
    _VIXEN_ALLOC_PROLOGUE(layout);

    usize size = allocation_size(layout);
    void *base_ptr
        = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base_ptr == MAP_FAILED) {
        throw allocation_exception{};
    }
    void *aligned_ptr = align_up(base_ptr, layout.align);

    // Large alignments will often leave a bunch of padding before the usable allocation, so we
    // just unmap that here so we don't have to keep track of anything for dealloc.
    if (base_ptr != aligned_ptr) {
        munmap(base_ptr, (usize)aligned_ptr - (usize)base_ptr);
    }

    return aligned_ptr;
}

void page_allocator::internal_dealloc(const layout &layout, void *ptr) {
    _VIXEN_DEALLOC_PROLOGUE(layout, ptr)

    // Note that munmap should never fail here because we will never split a mapped region, only
    // unmap the entire range.
    munmap(ptr, allocation_size(layout));
}

} // namespace vixen::heap
