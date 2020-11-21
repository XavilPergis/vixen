#include "xen/allocator/profile.hpp"

#include "xen/allocator/stacktrace.hpp"
#include "xen/hashmap.hpp"
#include "xen/option.hpp"
#include "xen/string.hpp"
#include "xen/vec.hpp"

#include <xen/traits.hpp>
#include <xen/types.hpp>

namespace xen::heap {

struct allocation_info {
    layout allocated_with;
    usize realloc_count;
    option<vector<void *>> stack_trace{};
};

void destroy(allocation_info &info) {
    destroy(info.stack_trace);
}

struct allocator_info {
    option<string> name;

    usize num_bytes_in_use = 0;
    usize maximum_bytes_in_use = 0;
    usize num_active_allocations = 0;
    usize maximum_active_allocations = 0;

    usize current_transaction_depth = 0;
    bool should_capture_stack_traces = true;

    vector<query_id> listening_queries{debug_allocator()};
    hash_map<void *, allocation_info> active_allocations{debug_allocator()};
};

struct internal_query_info {
    query_info query;
    allocator_id attached_to;
};

void destroy(allocator_info &info) {
    destroy(info.name);
    destroy(info.listening_queries);
    destroy(info.active_allocations);
}

static usize max_allocator_id = 0;
static usize max_query_id = 0;
static vector<allocator_id> freed_allocator_names(debug_allocator());
static vector<query_id> freed_query_names(debug_allocator());

static vector<allocator_info> allocator_infos(debug_allocator());
static vector<internal_query_info> queries(debug_allocator());

query_id create_memory_performace_query(allocator_id alloc_id) {
    query_id id;
    internal_query_info query_info;
    query_info.query = {};
    query_info.attached_to = alloc_id;
    if (freed_query_names.len() > 0) {
        id = *freed_query_names.pop();
        queries[id.id] = query_info;
    } else {
        id = {static_cast<isize>(max_query_id++)};
        queries.push(query_info);
    }
    allocator_infos[alloc_id.id].listening_queries.push(id);
    return id;
}

void delete_memory_performace_query(query_id id) {
    internal_query_info *query_info = &queries[id.id];
    allocator_info *affected_alloc_info = &allocator_infos[query_info->attached_to.id];

    if (auto idx = affected_alloc_info->listening_queries.index_of(id)) {
        affected_alloc_info->listening_queries.remove(*idx);
    }

    freed_query_names.push(id);
}

query_info measure_memory_performace_query(query_id id) {
    query_info info = queries[id.id].query;
    queries[id.id].query = {};
    return info;
}

void register_allocator(allocator *alloc) {
    if (freed_allocator_names.len() > 0) {
        alloc->id = *freed_allocator_names.pop();
        allocator_infos[alloc->id.id] = allocator_info{};
    } else {
        alloc->id = {static_cast<isize>(max_allocator_id++)};
        allocator_infos.push(allocator_info{});
    }
}

constexpr usize ONE_KB = 1024;
constexpr usize ONE_MB = ONE_KB * 1024;
constexpr usize ONE_GB = ONE_MB * 1024;
constexpr usize ONE_TB = ONE_GB * 1024;
constexpr usize ONE_PB = ONE_TB * 1024;

static const char *bytes_scale(usize bytes) {
    // clang-format off
    if      (bytes < ONE_KB) { return "B";   }
    else if (bytes < ONE_MB) { return "KiB"; }
    else if (bytes < ONE_GB) { return "MiB"; }
    else if (bytes < ONE_TB) { return "GiB"; }
    else if (bytes < ONE_PB) { return "TiB"; }
    // clang-format on

    XEN_ENGINE_UNREACHABLE("too many bytes!")
}

static usize bytes_units(usize bytes) {
    // clang-format off
    if      (bytes < ONE_KB) { return bytes;          }
    else if (bytes < ONE_MB) { return bytes / ONE_KB; }
    else if (bytes < ONE_GB) { return bytes / ONE_MB; }
    else if (bytes < ONE_TB) { return bytes / ONE_GB; }
    else if (bytes < ONE_PB) { return bytes / ONE_TB; }
    // clang-format on

    XEN_ENGINE_UNREACHABLE("too many bytes!")
}

void unregister_allocator(allocator *alloc) {
    allocator_info &info = allocator_infos[alloc->id.id];

    XEN_ENGINE_INFO("allocator stats for `{}`:", info.name ? *info.name : "<unknown>"_s)

    XEN_ENGINE_INFO("\tMaximum tracked bytes used: {} {} ({} bytes)",
        bytes_units(info.maximum_bytes_in_use),
        bytes_scale(info.maximum_bytes_in_use),
        info.maximum_bytes_in_use)

    XEN_ENGINE_INFO("\tCurrent tracked bytes used: {} {} ({} bytes)",
        bytes_units(info.num_bytes_in_use),
        bytes_scale(info.num_bytes_in_use),
        info.num_bytes_in_use)

    XEN_ENGINE_INFO("\tMaximum tracked active allocations: {}", info.maximum_active_allocations)
    XEN_ENGINE_INFO("\tCurrent tracked active allocations: {}", info.num_active_allocations)

    translation_cache cache(debug_allocator());

    if (info.active_allocations.len() > 0) {
        XEN_ENGINE_INFO("\tActive Allocations:")
    }
    info.active_allocations.iter([&](auto &ptr, auto &info) {
        XEN_ENGINE_WARN("\t\t- Pointer: {}", ptr)
        XEN_ENGINE_WARN("\t\t\t- layout: {}", info.allocated_with)
        XEN_ENGINE_WARN("\t\t\t- Realloc Count: {}", info.realloc_count)

        if (info.stack_trace) {
            vector<address_info> addr_infos = translate_stack_trace(&cache, *info.stack_trace);
            defer(destroy(addr_infos));
            print_stack_trace_capture(addr_infos);
        }
    });

    destroy(allocator_infos[alloc->id.id]);
    freed_allocator_names.push(alloc->id);
}

usize get_active_allocation_count(allocator_id id) {
    return allocator_infos[id.id].num_active_allocations;
}

usize get_active_allocation_max_count(allocator_id id) {
    return allocator_infos[id.id].maximum_active_allocations;
}

usize get_active_byte_count(allocator_id id) {
    return allocator_infos[id.id].num_bytes_in_use;
}

usize get_active_byte_max_count(allocator_id id) {
    return allocator_infos[id.id].maximum_bytes_in_use;
}

void set_allocator_name(allocator_id id, string_slice name) {
    allocator_info &info = allocator_infos[id.id];
    if (info.name.is_none()) {
        info.name = string(debug_allocator(), name);
    } else {
        info.name->clear();
        info.name->push(name);
    }
}

option<string_slice> get_allocator_name(allocator_id id) {
    option<string> &name = allocator_infos[id.id].name;
    return name ? option<string_slice>{*name} : option<string_slice>{};
}

// allocator_id set_allocator_parent(allocator_id child, allocator_id parent) {}

// allocator_id get_allocator_parent(allocator_id child) {}

void begin_transaction(allocator_id id) {
    if (debug_allocator()->id != id) {
        allocator_infos[id.id].current_transaction_depth += 1;
    }
}

void end_transaction(allocator_id id) {
    if (debug_allocator()->id != id) {
        allocator_infos[id.id].current_transaction_depth -= 1;
    }
}

static void commit_alloc(allocator_info *alloc_info, layout layout, void *ptr) {
    alloc_info->num_bytes_in_use += layout.size;
    alloc_info->num_active_allocations += 1;

    alloc_info->maximum_active_allocations
        = std::max(alloc_info->maximum_active_allocations, alloc_info->num_active_allocations);
    alloc_info->maximum_bytes_in_use
        = std::max(alloc_info->maximum_bytes_in_use, alloc_info->num_bytes_in_use);

    for (usize i = 0; i < alloc_info->listening_queries.len(); ++i) {
        internal_query_info *query = &queries[alloc_info->listening_queries[i].id];

        query->query.bytes_in_use += layout.size;
        query->query.cum_allocation_bytes += layout.size;
        query->query.active_allocations += 1;
        query->query.allocation_count += 1;

        query->query.maximum_active_allocations
            = std::max(query->query.maximum_active_allocations, query->query.active_allocations);
        query->query.maximum_bytes_in_use
            = std::max(query->query.maximum_bytes_in_use, query->query.bytes_in_use);
    }

    allocation_info info{layout, 0};
    if (alloc_info->should_capture_stack_traces) {
        info.stack_trace = capture_stack_trace(debug_allocator());
    }

    // TODO: check for overlapping allocations, not just the subset where the allocation starts at
    // an old start.
    option<allocation_info> prev = alloc_info->active_allocations.insert(ptr, MOVE(info));
    XEN_ENGINE_ASSERT(prev.is_none(),
        "Tried to allocate over a previous active allocation at {}",
        ptr)
}

static void commit_dealloc(allocator_info *alloc_info, layout layout, void *ptr) {
    alloc_info->num_bytes_in_use -= layout.size;
    alloc_info->num_active_allocations -= 1;

    for (usize i = 0; i < alloc_info->listening_queries.len(); ++i) {
        internal_query_info *query = &queries[alloc_info->listening_queries[i].id];

        query->query.bytes_in_use -= layout.size;
        query->query.cum_deallocation_bytes += layout.size;
        query->query.active_allocations -= 1;
        query->query.deallocation_count += 1;
    }

    option<allocation_info> prev = alloc_info->active_allocations.remove(ptr);
    XEN_ENGINE_ASSERT(prev.is_some(),
        "Tried to deallocate pointer at {} using layout {}, but the pointer was not an active allocation.",
        ptr,
        layout)

    XEN_ENGINE_ASSERT(
        prev->allocated_with.size == layout.size && prev->allocated_with.align == layout.align,
        "Tried to deallocate pointer at {} using {}, but the pointer was allocated using {}.",
        ptr,
        layout,
        prev->allocated_with)

    destroy(prev);
}

void record_reset(allocator_id id) {}

void record_alloc(allocator_id id, layout layout, void *ptr) {
    if (debug_allocator()->id == id) {
        return;
    }

    allocator_info *alloc_info = &allocator_infos[id.id];
    if (ptr == nullptr || alloc_info->current_transaction_depth != 1) {
        return;
    }

    XEN_ENGINE_TRACE("[A] {} ({})", layout, ptr)
    commit_alloc(alloc_info, layout, ptr);
}

void record_dealloc(allocator_id id, layout layout, void *ptr) {
    if (debug_allocator()->id == id) {
        return;
    }

    allocator_info *alloc_info = &allocator_infos[id.id];
    if (ptr == nullptr || alloc_info->current_transaction_depth != 1) {
        return;
    }

    XEN_ENGINE_TRACE("[D] {} ({})", layout, ptr)
    commit_dealloc(alloc_info, layout, ptr);
}

void record_realloc(
    allocator_id id, layout old_layout, void *old_ptr, layout new_layout, void *new_ptr) {
    if (debug_allocator()->id == id) {
        return;
    }
    allocator_info *alloc_info = &allocator_infos[id.id];
    if (alloc_info->current_transaction_depth != 1) {
        return;
    }

    if (old_ptr == nullptr && new_ptr != nullptr) {
        // Realloc zero -> something, which is an allocation.
        XEN_ENGINE_TRACE("[R:A] {} ({})", new_layout, new_ptr)
        commit_alloc(alloc_info, new_layout, new_ptr);
    } else if (old_ptr != nullptr && new_ptr == nullptr) {
        // Realloc something -> zero, which is a deallocation.
        XEN_ENGINE_TRACE("[R:D] {} ({})", old_layout, old_ptr)
        commit_dealloc(alloc_info, old_layout, old_ptr);
    } else if (old_ptr != nullptr, new_ptr != nullptr) {
        // Bona fide reallocation!
        XEN_ENGINE_TRACE("[R] {} ({}) -> {} ({})", old_layout, old_ptr, new_layout, old_layout)
        alloc_info->num_bytes_in_use -= old_layout.size;
        alloc_info->num_bytes_in_use += new_layout.size;
        alloc_info->maximum_bytes_in_use
            = std::max(alloc_info->maximum_bytes_in_use, alloc_info->num_bytes_in_use);

        for (usize i = 0; i < alloc_info->listening_queries.len(); ++i) {
            internal_query_info *query = &queries[alloc_info->listening_queries[i].id];

            query->query.bytes_in_use -= old_layout.size;
            query->query.bytes_in_use += new_layout.size;
            query->query.cum_allocation_bytes += new_layout.size;
            query->query.cum_deallocation_bytes += old_layout.size;
            query->query.reallocation_count += 1;

            query->query.maximum_bytes_in_use
                = std::max(query->query.maximum_bytes_in_use, query->query.bytes_in_use);
        }

        option<allocation_info> prev = alloc_info->active_allocations.remove(old_ptr);
        XEN_ENGINE_ASSERT(prev.is_some(),
            "Tried to reallocate pointer {}, but it was not an active allocation.",
            old_ptr)

        destroy(prev->stack_trace);
        allocation_info info{new_layout, prev->realloc_count + 1};
        if (alloc_info->should_capture_stack_traces) {
            info.stack_trace = capture_stack_trace(debug_allocator());
        }

        option<allocation_info> overlapping
            = alloc_info->active_allocations.insert(new_ptr, MOVE(info));
        XEN_ENGINE_ASSERT(overlapping.is_none(),
            "Tried to reallocate pointer {} ({}) to {} ({}), but it overlaps with an active allocation with layout ({}).",
            old_ptr,
            old_layout,
            new_ptr,
            new_layout,
            overlapping->allocated_with)
    }
}

void record_legacy_alloc(allocator_id id, usize size, void *ptr) {}

void record_legacy_dealloc(allocator_id id, void *ptr) {}

void record_legacy_realloc(allocator_id id, void *old_ptr, usize new_size, void *new_ptr) {}

} // namespace xen::heap
