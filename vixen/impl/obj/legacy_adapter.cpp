#include "vixen/allocator/allocators.hpp"
#include "vixen/util.hpp"

namespace vixen::heap {

static usize read_alloc_size(void *raw) {
    usize size;
    util::copy_nonoverlapping_from_raw(raw, &size, 1);
    return size;
}

static void write_alloc_size(void *raw, usize size) {
    util::copy_nonoverlapping_to_raw(&size, raw, 1);
}

static layout legacy_layout(usize legacy_size) {
    return {legacy_size + MAX_LEGACY_ALIGNMENT, MAX_LEGACY_ALIGNMENT};
}

static_assert(MAX_LEGACY_ALIGNMENT >= sizeof(usize));

void *legacy_adapter_allocator::internal_legacy_alloc(usize size) {
    void *raw = adapted->alloc(legacy_layout(size));
    write_alloc_size(raw, size);
    return void_ptr_add(raw, MAX_LEGACY_ALIGNMENT);
}

void legacy_adapter_allocator::internal_legacy_dealloc(void *ptr) {
    if (ptr == nullptr)
        return;

    void *raw = void_ptr_sub(ptr, MAX_LEGACY_ALIGNMENT);
    usize size = read_alloc_size(raw);

    adapted->dealloc(legacy_layout(size), raw);
}

void *legacy_adapter_allocator::internal_legacy_realloc(usize new_size, void *old_ptr) {
    if (old_ptr == nullptr) {
        return internal_legacy_alloc(new_size);
    } else if (new_size == 0) {
        internal_legacy_dealloc(old_ptr);
        return nullptr;
    } else {
        void *old_raw = void_ptr_sub(old_ptr, MAX_LEGACY_ALIGNMENT);
        usize old_size = read_alloc_size(old_raw);

        void *new_raw = adapted->realloc(legacy_layout(old_size), legacy_layout(new_size), old_raw);
        write_alloc_size(new_raw, new_size);
        return void_ptr_add(new_raw, MAX_LEGACY_ALIGNMENT);
    }
}

} // namespace vixen::heap
