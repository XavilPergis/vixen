#include "vixen/allocator/allocators.hpp"
#include "vixen/util.hpp"

namespace vixen::heap {

static usize readAllocSize(void *raw) {
    usize size;
    util::arrayCopyNonoverlappingFromRaw(raw, &size, 1);
    return size;
}

static void writeAllocSize(void *raw, usize size) {
    new (raw) usize(size);
}

static Layout legacyLayout(usize legacySize) {
    return {legacySize + MAX_LEGACY_ALIGNMENT, MAX_LEGACY_ALIGNMENT};
}

static_assert(MAX_LEGACY_ALIGNMENT >= sizeof(usize));

void *LegacyAdapterAllocator::internalLegacyAlloc(usize size) {
    void *raw = adapted->alloc(legacyLayout(size));
    writeAllocSize(raw, size);
    return util::offsetRaw(raw, MAX_LEGACY_ALIGNMENT);
}

void LegacyAdapterAllocator::internalLegacyDealloc(void *ptr) {
    if (ptr == nullptr) return;

    void *raw = util::offsetRaw(ptr, -MAX_LEGACY_ALIGNMENT);
    usize size = readAllocSize(raw);

    adapted->dealloc(legacyLayout(size), raw);
}

void *LegacyAdapterAllocator::internalLegacyRealloc(usize newSize, void *oldPtr) {
    if (oldPtr == nullptr) {
        return internalLegacyAlloc(newSize);
    } else if (newSize == 0) {
        internalLegacyDealloc(oldPtr);
        return nullptr;
    } else {
        void *oldRaw = util::offsetRaw(oldPtr, -MAX_LEGACY_ALIGNMENT);
        usize oldSize = readAllocSize(oldRaw);

        void *newRaw = adapted->realloc(legacyLayout(oldSize), legacyLayout(newSize), oldRaw);
        writeAllocSize(newRaw, newSize);
        return util::offsetRaw(newRaw, MAX_LEGACY_ALIGNMENT);
    }
}

} // namespace vixen::heap
