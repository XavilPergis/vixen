#include "vixen/allocator/allocators.hpp"
#include "vixen/allocator/profile.hpp"
#include "vixen/assert.hpp"

#if defined(VIXEN_PLATFORM_LINUX)
#include <unistd.h>
#elif defined(VIXEN_PLATFORM_WINDOWS)
#include <sysinfoapi.h>
#endif

namespace vixen::heap {

Allocator &globalAllocator() {
    static SystemAllocator gGlobalAllocator{};
    return gGlobalAllocator;
}

LegacyAllocator &legacyGlobalAllocator() {
    static LegacyAdapterAllocator gGlobalAdapter{globalAllocator()};
    return gGlobalAdapter;
}

Allocator &debugAllocator() {
    static SystemAllocator gDebugAllocator{};
    return gDebugAllocator;
}

#if defined(VIXEN_PLATFORM_LINUX)
usize pageSize() {
    static usize gPageSize = sysconf(_SC_PAGE_SIZE);
    return gPageSize;
}
#elif defined(VIXEN_PLATFORM_WINDOWS)
static usize findPageSize() {
    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    return si.dwPageSize;
}

usize pageSize() {
    static usize gPageSize = findPageSize();
    return gPageSize;
}
#endif

// void *allocNextLayer(AllocatorLayer *layer, Allocator &alloc, const Layout &layout) {
//     return layer ? layer->alloc(alloc, layout) : alloc.internalAlloc(layout);
// }

LayerExecutor::LayerExecutor(Allocator *allocator)
    : allocator(allocator), currentLayer(allocator->layerHead) {}

AllocatorLayer *LayerExecutor::advanceLayer() {
    AllocatorLayer *layer = this->currentLayer;

    if (this->currentLayer != nullptr) {
        this->currentLayer = this->currentLayer->nextLayer;
    } else if (this->currentState == State::EXECUTING_LOCAL) {
        this->currentState = State::EXECUTING_GLOBAL;
        this->currentLayer = getGlobalLayerHead();
        layer = this->currentLayer;
    }

    return layer;
}

void *LayerExecutor::alloc(Layout const &layout) {
    auto *layer = this->advanceLayer();
    if (layer == nullptr) return this->allocator->internalAlloc(layout);
    return this->currentLayer->alloc(*this, layout);
}

void LayerExecutor::dealloc(Layout const &layout, void *ptr) {
    auto *layer = this->advanceLayer();
    if (layer == nullptr) return this->allocator->internalDealloc(layout, ptr);
    this->currentLayer->dealloc(*this, layout, ptr);
}

void *LayerExecutor::realloc(Layout const &oldLayout, Layout const &newLayout, void *oldPtr) {
    auto *layer = this->advanceLayer();
    if (layer == nullptr) return this->allocator->internalRealloc(oldLayout, newLayout, oldPtr);
    return this->currentLayer->realloc(*this, oldLayout, newLayout, oldPtr);
}

void LayerExecutor::reset() {
    auto *layer = this->advanceLayer();
    if (layer == nullptr) {
        // NOTE: CAST: This cast is okay because we require as a precondition that a
        // `ResettableAllocator` object lives in `this->allocator`.
        auto *resettable = static_cast<ResettableAllocator *>(this->allocator);
        return resettable->internalReset();
    }
    return this->currentLayer->reset(*this);
}

void *LayerExecutor::legacyAlloc(usize size) {
    auto *layer = this->advanceLayer();
    if (layer == nullptr) {
        // NOTE: CAST: see note in `LayerExecutor::reset`
        auto *legacy = static_cast<LegacyAllocator *>(this->allocator);
        return legacy->legacyAlloc(size);
    }
    return this->currentLayer->legacyAlloc(*this, size);
}

void LayerExecutor::legacyDealloc(void *ptr) {
    auto *layer = this->advanceLayer();
    if (layer == nullptr) {
        // NOTE: CAST: see note in `LayerExecutor::reset`
        auto *legacy = static_cast<LegacyAllocator *>(this->allocator);
        return legacy->legacyDealloc(ptr);
    }
    return this->currentLayer->legacyDealloc(*this, ptr);
}

void *LayerExecutor::legacyRealloc(usize size, void *ptr) {
    auto *layer = this->advanceLayer();
    if (layer == nullptr) {
        // NOTE: CAST: see note in `LayerExecutor::reset`
        auto *legacy = static_cast<LegacyAllocator *>(this->allocator);
        return legacy->legacyRealloc(size, ptr);
    }
    return this->currentLayer->legacyRealloc(*this, size, ptr);
}

void *ClearMemoryLayer::alloc(LayerExecutor &exec, Layout const &layout) {
    void *res = exec.alloc(layout);
    util::fillUninitialized(ALLOCATION_PATTERN, static_cast<u8 *>(res), layout.size);
    return res;
}

void ClearMemoryLayer::dealloc(LayerExecutor &exec, const Layout &layout, void *ptr) {
    util::fillUninitialized(DEALLOCATION_PATTERN, static_cast<u8 *>(ptr), layout.size);
    exec.dealloc(layout, ptr);
}

void *Allocator::alloc(const Layout &layout) {
    LayerExecutor exec{this};
    return exec.alloc(layout);
}

void Allocator::dealloc(const Layout &layout, void *ptr) {
    VIXEN_ASSERT(ptr != nullptr);

    LayerExecutor exec{this};
    exec.dealloc(layout, ptr);
}

void *Allocator::realloc(const Layout &oldLayout, const Layout &newLayout, void *oldPtr) {
    LayerExecutor exec{this};
    return exec.realloc(oldLayout, newLayout, oldPtr);
}

void ResettableAllocator::reset() {
    LayerExecutor exec{this};
    exec.reset();
}

void *LegacyAllocator::internalAlloc(const Layout &layout) {
    _VIXEN_ALLOC_PROLOGUE(layout)

    if (layout.align <= MAX_LEGACY_ALIGNMENT) {
        return internalLegacyAlloc(layout.size);
    } else {
        VIXEN_PANIC(
            "Tried to allocate {} on LegacyAllocator, which does not support alignments above {}.",
            layout,
            MAX_LEGACY_ALIGNMENT);
    }
}

void LegacyAllocator::internalDealloc(const Layout &layout, void *ptr) {
    _VIXEN_DEALLOC_PROLOGUE(layout, ptr)

    internalLegacyDealloc(ptr);
}

void *LegacyAllocator::legacyAlloc(usize size) {
    VIXEN_ASSERT_EXT(this != nullptr,
        "Tried to legacy allocate {} bytes, but the allocator pointer was null.",
        size);
    // begin_transaction(id);
    void *ptr = this->internalLegacyAlloc(size);
    // record_legacy_alloc(id, size, ptr);
    // end_transaction(id);
    util::fillUninitialized(ALLOCATION_PATTERN, static_cast<u8 *>(ptr), size);
    return ptr;
}

void LegacyAllocator::legacyDealloc(void *ptr) {
    VIXEN_ASSERT_EXT(this != nullptr,
        "Tried to legacy deallocate {}, but the allocator pointer was null.",
        ptr);
    // begin_transaction(id);
    this->internalLegacyDealloc(ptr);
    // record_legacy_dealloc(id, ptr);
    // end_transaction(id);
}

void *LegacyAllocator::legacyRealloc(usize new_size, void *oldPtr) {
    VIXEN_ASSERT_EXT(this != nullptr,
        "Tried to legacy reallocate {} to new size of {} bytes, but the allocator pointer was null.",
        oldPtr,
        new_size);
    // begin_transaction(id);
    void *ptr = this->internalLegacyRealloc(new_size, oldPtr);
    // record_legacy_realloc(id, oldPtr, new_size, ptr);
    // end_transaction(id);
    // TODO: debug memsetting!
    return ptr;
}

} // namespace vixen::heap
