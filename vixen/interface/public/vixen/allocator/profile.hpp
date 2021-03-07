#pragma once

#include "vixen/allocator/layout.hpp"
#include "vixen/types.hpp"

/// @file
/// @ingroup vixen_allocator

namespace vixen {
struct StringView;
template <typename T>
struct Option;
} // namespace vixen

namespace vixen::heap {

struct Allocator;

struct AllocatorId {
    isize id;

    bool operator==(const AllocatorId &other) const {
        return id == other.id;
    }

    bool operator!=(const AllocatorId &other) const {
        return id != other.id;
    }
};

struct QueryId {
    isize id;

    bool operator==(const QueryId &other) const {
        return id == other.id;
    }

    bool operator!=(const QueryId &other) const {
        return id != other.id;
    }
};

struct QueryInfo {
    usize bytes_in_use = 0;
    usize active_allocations = 0;
    usize maximum_bytes_in_use = 0;
    usize maximum_active_allocations = 0;

    usize allocation_count = 0;
    usize deallocation_count = 0;
    usize reallocation_count = 0;

    usize cum_allocation_bytes = 0;
    usize cum_deallocation_bytes = 0;
};

constexpr AllocatorId NOT_TRACKED_ID = {-1};

void register_allocator(Allocator *alloc);
void unregister_allocator(Allocator *alloc);

void set_allocator_name(AllocatorId id, StringView name);
Option<StringView> get_allocator_name(AllocatorId id);

// allocator_id set_allocator_parent(allocator_id child, allocator_id parent);
// allocator_id get_allocator_parent(allocator_id child);

void begin_transaction(AllocatorId id);
void end_transaction(AllocatorId id);

void record_reset(AllocatorId id);
void record_alloc(AllocatorId id, Layout layout, void *ptr);
void record_dealloc(AllocatorId id, Layout layout, void *ptr);
void record_realloc(
    AllocatorId id, Layout old_layout, void *old_ptr, Layout new_layout, void *new_ptr);

void record_legacy_alloc(AllocatorId id, usize size, void *ptr);
void record_legacy_dealloc(AllocatorId id, void *ptr);
void record_legacy_realloc(AllocatorId id, void *old_ptr, usize new_size, void *new_ptr);

QueryId create_memory_performace_query(AllocatorId id);
QueryInfo measure_memory_performace_query(QueryId id);
void delete_memory_performace_query(QueryId id);

usize get_active_allocation_count(AllocatorId id);
usize get_active_allocation_max_count(AllocatorId id);
usize get_active_byte_count(AllocatorId id);
usize get_active_byte_max_count(AllocatorId id);

} // namespace vixen::heap
