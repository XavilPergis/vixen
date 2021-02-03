#pragma once

#include "vixen/allocator/layout.hpp"
#include "vixen/types.hpp"

/// @file
/// @ingroup vixen_allocator

namespace vixen {
struct string_slice;
template <typename T>
struct option;
} // namespace vixen

namespace vixen::heap {

struct allocator;

struct allocator_id {
    isize id;

    bool operator==(const allocator_id &other) const {
        return id == other.id;
    }

    bool operator!=(const allocator_id &other) const {
        return id != other.id;
    }
};

struct query_id {
    isize id;

    bool operator==(const query_id &other) const {
        return id == other.id;
    }

    bool operator!=(const query_id &other) const {
        return id != other.id;
    }
};

struct query_info {
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

constexpr allocator_id NOT_TRACKED_ID = {-1};

void register_allocator(allocator *alloc);
void unregister_allocator(allocator *alloc);

void set_allocator_name(allocator_id id, string_slice name);
option<string_slice> get_allocator_name(allocator_id id);

// allocator_id set_allocator_parent(allocator_id child, allocator_id parent);
// allocator_id get_allocator_parent(allocator_id child);

void begin_transaction(allocator_id id);
void end_transaction(allocator_id id);

void record_reset(allocator_id id);
void record_alloc(allocator_id id, layout layout, void *ptr);
void record_dealloc(allocator_id id, layout layout, void *ptr);
void record_realloc(
    allocator_id id, layout old_layout, void *old_ptr, layout new_layout, void *new_ptr);

void record_legacy_alloc(allocator_id id, usize size, void *ptr);
void record_legacy_dealloc(allocator_id id, void *ptr);
void record_legacy_realloc(allocator_id id, void *old_ptr, usize new_size, void *new_ptr);

query_id create_memory_performace_query(allocator_id id);
query_info measure_memory_performace_query(query_id id);
void delete_memory_performace_query(query_id id);

usize get_active_allocation_count(allocator_id id);
usize get_active_allocation_max_count(allocator_id id);
usize get_active_byte_count(allocator_id id);
usize get_active_byte_max_count(allocator_id id);

} // namespace vixen::heap
