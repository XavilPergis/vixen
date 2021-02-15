#include "vixen/allocator/profile.hpp"

#include "vixen/allocator/stacktrace.hpp"
#include "vixen/common.hpp"
#include "vixen/stream.hpp"
#include "vixen/traits.hpp"
#include "vixen/types.hpp"

#include <thread>

namespace vixen::heap {

struct allocation_info {
    allocation_info(rawptr ptr, layout layout) : base(ptr), allocated_with(layout) {}

    allocation_info(allocator *alloc, const allocation_info &other)
        : base(other.base)
        , allocated_with(other.allocated_with)
        , realloc_count(other.realloc_count) {
        if (other.stack_trace) {
            stack_trace = other.stack_trace->clone(alloc);
        }
    }

    rawptr base;
    layout allocated_with;

    usize realloc_count{0};
    option<vector<void *>> stack_trace{};
};

#define BUCKET_SIZE 16384

usize get_bucket_id(rawptr addr) {
    return (usize)addr / BUCKET_SIZE;
}

struct allocation_range {
    rawptr start, end;
};

struct allocation_bucket {
    allocation_bucket(allocator *alloc, rawptr start, rawptr end)
        : start(start), end(end), allocations(alloc) {}

    rawptr start, end;
    vector<allocation_range> allocations;
};

rawptr range_start(const allocation_range &range) {
    return range.start;
}

usize bucket_start(const allocation_bucket &bucket) {
    return get_bucket_id(bucket.start);
}

// this is SO BAD
struct allocation_checker {
    allocator *alloc;
    vector<allocation_bucket> buckets;
    hash_map<rawptr, allocation_info> infos;

    allocation_checker(allocator *alloc) : alloc(alloc), buckets(alloc), infos(alloc) {}

    void add_range_at(rawptr addr, allocation_range range) {
        auto bucket_idx
            = binary_search(buckets.begin(), buckets.end(), get_bucket_id(addr), bucket_start);
        if (bucket_idx.is_err()) {
            allocation_bucket bucket_to_add(alloc, addr, util::offset_rawptr(addr, BUCKET_SIZE));
            buckets.shift_insert(bucket_idx.unwrap_err(), mv(bucket_to_add));
        }

        auto &bucket = buckets[unify_result(mv(bucket_idx))];
        auto range_idx = binary_search(bucket.allocations.begin(),
            bucket.allocations.end(),
            range.start,
            range_start);
        VIXEN_ASSERT_EXT(range_idx.is_err(), "tried to insert already-existent range");
        bucket.allocations.shift_insert(range_idx.unwrap_err(), range);
    }

    option<const allocation_info &> add(rawptr ptr, allocation_info &&info) {
        rawptr start_addr = ptr;
        rawptr end_addr = util::offset_rawptr(start_addr, info.allocated_with.size);
        auto start_bucket_addr = get_bucket_id(start_addr);
        auto end_bucket_addr = get_bucket_id(end_addr);
        allocation_range range{start_addr, end_addr};

        // VIXEN_WARN("adding {}-{} with layout {}", start_addr, end_addr, info.allocated_with);

        if (auto overlapping = get_overlapping_range(range)) {
            // VIXEN_WARN("overlapping {}-{} ", overlapping->start, overlapping->end);
            return infos[overlapping->start];
        }

        add_range_at(start_addr, range);
        if (start_bucket_addr != end_bucket_addr) {
            add_range_at(end_addr, range);
        }

        infos.insert(start_addr, mv(info));
        return empty_opt;
    }

    struct removal_info {
        option<allocation_info> info;
        bool is_dealloc_start_misaligned;
        bool is_dealloc_end_misaligned;
    };

    removal_info remove(rawptr addr, layout layout, allocator *info_clone_alloc) {
        rawptr start_addr = addr;
        rawptr end_addr = util::offset_rawptr(start_addr, layout.size);
        allocation_range range{start_addr, end_addr};

        // VIXEN_WARN("removing {}", addr);

        if (auto overlapping_range = get_overlapping_range(addr)) {
            if (range.start == overlapping_range->start && range.end == overlapping_range->end) {
                auto start_bucket_idx = get_bucket_index(range.start);
                auto start_range_idx = get_range_index(*start_bucket_idx, range.start);
                buckets[*start_bucket_idx].allocations.shift_remove(*start_range_idx);

                auto end_bucket_idx = get_bucket_index(range.end);
                if (start_bucket_idx != end_bucket_idx) {
                    auto end_range_idx = get_range_index(*end_bucket_idx, range.start);
                    buckets[*end_bucket_idx].allocations.shift_remove(*end_range_idx);
                }

                return {infos.remove(overlapping_range->start), false, false};
            } else {
                return {allocation_info{info_clone_alloc, infos[overlapping_range->start]},
                    overlapping_range->start != start_addr,
                    overlapping_range->end != end_addr};
            }
        }

        return {empty_opt, false, false};
    }

    option<usize> get_bucket_index(rawptr addr) const {
        return binary_search(buckets.begin(), buckets.end(), get_bucket_id(addr), bucket_start)
            .to_ok();
    }

    option<usize> get_range_index(usize bucket_idx, rawptr addr) const {
        return binary_search(buckets[bucket_idx].allocations.begin(),
            buckets[bucket_idx].allocations.end(),
            addr,
            range_start)
            .to_ok();
    }

    bool remove_range(allocation_range range) {
        auto start_bucket_index = binary_search(buckets.begin(),
            buckets.end(),
            get_bucket_id(range.start),
            bucket_start);

        if (start_bucket_index.is_ok()) {
            auto &start_bucket = buckets[start_bucket_index.unwrap_ok()];
            auto start_range_index = binary_search(start_bucket.allocations.begin(),
                start_bucket.allocations.end(),
                range.start,
                range_start);

            if (start_range_index.is_ok()) {
                auto end_bucket_index = binary_search(buckets.begin(),
                    buckets.end(),
                    get_bucket_id(range.end),
                    bucket_start);
                auto &end_bucket = buckets[end_bucket_index.unwrap_ok()];
                auto end_range_index = binary_search(end_bucket.allocations.begin(),
                    end_bucket.allocations.end(),
                    range.start,
                    range_start);

                if (start_bucket.allocations[start_range_index.unwrap_ok()].end != range.end) {
                    return false;
                }

                start_bucket.allocations.shift_remove(start_range_index.unwrap_ok());
                end_bucket.allocations.shift_remove(end_range_index.unwrap_ok());

                return true;
            }
        }

        return false;
    }

    option<allocation_range> get_overlapping_range(rawptr addr) const {
        if (auto alloc_range = get_previous_range(addr)) {
            if (alloc_range->start <= addr && alloc_range->end > addr) {
                return alloc_range;
            }
        }
        return empty_opt;
    }

    option<allocation_range> get_overlapping_range(allocation_range range) const {
        auto prev_from_end = get_previous_range(range.end);
        if (prev_from_end.is_none())
            return empty_opt;

        if (range.start < prev_from_end->end && prev_from_end->start < range.end) {
            return prev_from_end;
        } else {
            return empty_opt;
        }
    }

    option<allocation_range> get_previous_range(rawptr addr) const {
        auto bucket_addr = get_bucket_id(addr);
        if (auto bucket_idx
            = binary_find_previous_index(buckets.begin(), buckets.end(), bucket_addr, bucket_start))
        {
            const auto &bucket = buckets[*bucket_idx];
            if (auto range_idx = binary_find_previous_index(bucket.allocations.begin(),
                    bucket.allocations.end(),
                    addr,
                    range_start))
            {
                return bucket.allocations[*range_idx];
            }
        } else if (bucket_addr > 0) {
            if (auto bucket_idx = binary_find_previous_index(buckets.begin(),
                    buckets.end(),
                    bucket_addr - 1,
                    bucket_start))
            {
                const auto &bucket = buckets[*bucket_idx];
                if (auto range_idx = binary_find_previous_index(bucket.allocations.begin(),
                        bucket.allocations.end(),
                        addr,
                        range_start))
                {
                    return bucket.allocations[*range_idx];
                }
            }
        }
        return empty_opt;
    }

    allocation_info &get_info(rawptr ptr) {
        return infos[ptr];
    }

    usize count() const {
        return infos.len();
    }
};

struct allocator_info {
    allocator_info(allocator *alloc) : listening_queries(alloc), checker(alloc) {}

    option<string> name;

    usize num_bytes_in_use = 0;
    usize maximum_bytes_in_use = 0;
    usize num_active_allocations = 0;
    usize maximum_active_allocations = 0;

    usize current_transaction_depth = 0;
    bool should_capture_stack_traces = true;

    vector<query_id> listening_queries;
    allocation_checker checker;
};

struct internal_query_info {
    query_info query;
    allocator_id attached_to;
};

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
        allocator_infos[alloc->id.id] = allocator_info(debug_allocator());
    } else {
        alloc->id = {static_cast<isize>(max_allocator_id++)};
        allocator_infos.push(allocator_info(debug_allocator()));
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

    VIXEN_UNREACHABLE("too many bytes!");
}

static usize bytes_units(usize bytes) {
    // clang-format off
    if      (bytes < ONE_KB) { return bytes;          }
    else if (bytes < ONE_MB) { return bytes / ONE_KB; }
    else if (bytes < ONE_GB) { return bytes / ONE_MB; }
    else if (bytes < ONE_TB) { return bytes / ONE_GB; }
    else if (bytes < ONE_PB) { return bytes / ONE_TB; }
    // clang-format on

    VIXEN_UNREACHABLE("too many bytes!");
}

void unregister_allocator(allocator *alloc) {
    allocator_info &info = allocator_infos[alloc->id.id];

    VIXEN_INFO("allocator stats for `{}`:", info.name ? *info.name : "<unknown>"_s);

    VIXEN_INFO("\tMaximum tracked bytes used: {} {} ({} bytes)",
        bytes_units(info.maximum_bytes_in_use),
        bytes_scale(info.maximum_bytes_in_use),
        info.maximum_bytes_in_use);

    VIXEN_INFO("\tCurrent tracked bytes used: {} {} ({} bytes)",
        bytes_units(info.num_bytes_in_use),
        bytes_scale(info.num_bytes_in_use),
        info.num_bytes_in_use);

    VIXEN_INFO("\tMaximum tracked active allocations: {}", info.maximum_active_allocations);
    VIXEN_INFO("\tCurrent tracked active allocations: {}", info.num_active_allocations);

    translation_cache cache(debug_allocator());

    // if (info.checker.count() > 0) {
    //     VIXEN_INFO("\tActive Allocations:");
    // }
    // info.checker.infos.iter([&](auto &ptr, auto &info) {
    //     VIXEN_WARN("\t\t- Pointer: {}", ptr);
    //     VIXEN_WARN("\t\t\t- layout: {}", info.allocated_with);
    //     VIXEN_WARN("\t\t\t- Realloc Count: {}", info.realloc_count);

    //     if (info.stack_trace) {
    //         vector<address_info> addr_infos = translate_stack_trace(&cache, *info.stack_trace);
    //         print_stack_trace_capture(addr_infos);
    //     }
    // });

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
    if (id != debug_allocator()->id && id != NOT_TRACKED_ID) {
        allocator_infos[id.id].current_transaction_depth += 1;
    }
}

void end_transaction(allocator_id id) {
    if (id != debug_allocator()->id && id != NOT_TRACKED_ID) {
        allocator_infos[id.id].current_transaction_depth -= 1;
    }
}

template <typename Stream>
void format_stack_trace(
    Stream &stream, translation_cache &cache, const slice<void *> &stack_trace) {
    auto translated = translate_stack_trace(&cache, stack_trace);
    for (usize i = 0; i < translated.len(); ++i) {
        format_address_info(stream, translated[i], i + 1);
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

    allocation_info info(ptr, layout);
    if (alloc_info->should_capture_stack_traces) {
        info.stack_trace = capture_stack_trace(debug_allocator());
    }

    if (auto overlapping = alloc_info->checker.add(ptr, mv(info))) {
        if (alloc_info->name) {
            VIXEN_PANIC(
                "allocation collision: tried to allocate over a previous allocation at {} in allocator '{}'.\n",
                overlapping->base,
                *alloc_info->name);
        } else {
            VIXEN_PANIC(
                "allocation collision: tried to allocate over a previous allocation at {}.\n",
                overlapping->base);
        }
        // VIXEN_PANIC(
        //     "allocation collision: tried to allocate over a previous allocation at {}:\n"
        //     "previous allocation info:\n"
        //     "\tat {}\n",
        //     "\tlayout {}\n",
        //     overlapping->base,
        //     overlapping->base,
        //     overlapping->allocated_with);
    }
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

    // option<allocation_info> prev = alloc_info->checker.infos.remove(ptr);

    // VIXEN_ASSERT_EXT(
    //     prev->allocated_with.size == layout.size && prev->allocated_with.align == layout.align,
    //     "Tried to deallocate pointer at {} using {}, but the pointer was allocated using {}.",
    //     ptr,
    //     layout,
    //     prev->allocated_with);

    auto removal_info = alloc_info->checker.remove(ptr, layout, debug_allocator());
    if (auto &info = removal_info.info) {
        if (removal_info.is_dealloc_start_misaligned || removal_info.is_dealloc_end_misaligned) {
            vector<char> diagnostic(debug_allocator());
            auto bi = stream::back_inserter(diagnostic);
            auto oi = stream::make_stream_output_iterator(bi);

            fmt::format_to(oi, "tried to deallocate, but the deallocation request misaligned:\n");

            fmt::format_to(oi, "request:\n");
            if (alloc_info->name) {
                fmt::format_to(oi, "- allocator name = '{}'\n", *alloc_info->name);
            }
            fmt::format_to(oi, "- pointer = {}\n", ptr);
            fmt::format_to(oi, "- layout = {}\n", layout);
            fmt::format_to(oi,
                "- start misaligned = {}\n",
                removal_info.is_dealloc_start_misaligned);
            fmt::format_to(oi, "- end misaligned = {}\n", removal_info.is_dealloc_end_misaligned);
            fmt::format_to(oi, "\n");
            fmt::format_to(oi, "block in:\n");
            fmt::format_to(oi, "- pointer = {}\n", info->base);
            fmt::format_to(oi, "- layout = {}\n", info->allocated_with);
            fmt::format_to(oi, "- reallocation count = {}\n", info->realloc_count);

            if (info->stack_trace) {
                fmt::format_to(oi, "- stack trace:\n");
                translation_cache cache(heap::debug_allocator());
                format_stack_trace(bi, cache, *info->stack_trace);
            }

            string diagnostic_str(mv(diagnostic));
            VIXEN_PANIC("{}", diagnostic_str);
        }
    } else {
        VIXEN_PANIC(
            "tried to deallocate pointer at {} using layout {}, but the pointer was not in any active allocation.",
            ptr,
            layout);
    }
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

    if (alloc_info->name) {
        VIXEN_TRACE("[A] '{}' {} ({})", *alloc_info->name, layout, ptr);
    } else {
        VIXEN_TRACE("[A] {} ({})", layout, ptr);
    }
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

    if (alloc_info->name) {
        VIXEN_TRACE("[D] '{}' {} ({})", *alloc_info->name, layout, ptr);
    } else {
        VIXEN_TRACE("[D] {} ({})", layout, ptr);
    }
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
        if (alloc_info->name) {
            VIXEN_TRACE("[R:A] '{}' {} ({})", *alloc_info->name, new_layout, new_ptr);
        } else {
            VIXEN_TRACE("[R:A] {} ({})", new_layout, new_ptr);
        }
        commit_alloc(alloc_info, new_layout, new_ptr);
    } else if (old_ptr != nullptr && new_ptr == nullptr) {
        // Realloc something -> zero, which is a deallocation.
        if (alloc_info->name) {
            VIXEN_TRACE("[R:D] '{}' {} ({})", *alloc_info->name, old_layout, old_ptr);
        } else {
            VIXEN_TRACE("[R:D] {} ({})", old_layout, old_ptr);
        }
        commit_dealloc(alloc_info, old_layout, old_ptr);
    } else if (old_ptr != nullptr && new_ptr != nullptr) {
        // Bona fide reallocation!
        if (alloc_info->name) {
            VIXEN_TRACE("[R] '{}' {} ({}) -> {} ({})",
                *alloc_info->name,
                old_layout,
                old_ptr,
                new_layout,
                old_layout);
        } else {
            VIXEN_TRACE("[R] {} ({}) -> {} ({})", old_layout, old_ptr, new_layout, old_layout);
        }
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

        auto removal_info = alloc_info->checker.remove(old_ptr, old_layout, debug_allocator());
        if (auto &info = removal_info.info) {
            if (removal_info.is_dealloc_start_misaligned || removal_info.is_dealloc_end_misaligned)
            {
                vector<char> diagnostic(debug_allocator());
                auto bi = stream::back_inserter(diagnostic);
                auto oi = stream::make_stream_output_iterator(bi);

                fmt::format_to(oi,
                    "tried to reallocate, but the allocated pointer was in the middle of a block:\n");

                fmt::format_to(oi, "request:\n");
                if (alloc_info->name) {
                    fmt::format_to(oi, "- allocator name = '{}'\n", *alloc_info->name);
                }
                fmt::format_to(oi, "- old pointer = {}\n", old_ptr);
                fmt::format_to(oi, "- old layout = {}\n", old_layout);
                fmt::format_to(oi, "- new pointer = {}\n", new_ptr);
                fmt::format_to(oi, "- new layout = {}\n", new_layout);
                fmt::format_to(oi,
                    "- start misaligned = {}\n",
                    removal_info.is_dealloc_start_misaligned);
                fmt::format_to(oi,
                    "- end misaligned = {}\n",
                    removal_info.is_dealloc_end_misaligned);
                fmt::format_to(oi, "\n");
                fmt::format_to(oi, "block in:\n");
                fmt::format_to(oi, "- pointer = {}\n", info->base);
                fmt::format_to(oi, "- layout = {}\n", info->allocated_with);
                fmt::format_to(oi, "- reallocation count = {}\n", info->realloc_count);

                if (info->stack_trace) {
                    fmt::format_to(oi, "- stack trace:\n");
                    translation_cache cache(heap::debug_allocator());
                    format_stack_trace(bi, cache, *info->stack_trace);
                }

                string diagnostic_str(mv(diagnostic));
                VIXEN_PANIC("{}", diagnostic_str);
            } else {
                // TODO: maybe record stack traces for reallocations too instead of only keeping
                // track of the initial alloc.
                info->allocated_with = new_layout;
                info->base = new_ptr;
                if (auto overlapping = alloc_info->checker.add(new_ptr, mv(*info))) {
                    VIXEN_PANIC(
                        "allocation collision: tried to allocate over a previous allocation at {}.\n",
                        overlapping->base);
                }
            }
        } else {
            VIXEN_PANIC(
                "tried to reallocate, but the old pointer was not in any active allocation:\n"
                "reallocation request:\n",
                "old pointer = {}\n"
                "old layout = {}\n"
                "new pointer = {}\n"
                "new layout = {}\n",
                old_ptr,
                old_layout,
                new_ptr,
                new_layout);
        }

        // option<allocation_info> prev = alloc_info->checker.infos.remove(old_ptr);
        // VIXEN_ASSERT_EXT(prev.is_some(),
        //     "Tried to reallocate pointer {}, but it was not an active allocation.",
        //     old_ptr);

        // allocation_info info{new_layout, prev->realloc_count + 1};
        // if (alloc_info->should_capture_stack_traces) {
        //     info.stack_trace = capture_stack_trace(debug_allocator());
        // }

        // option<allocation_info> overlapping = alloc_info->checker.infos.insert(new_ptr,
        // mv(info)); VIXEN_ASSERT_EXT(overlapping.is_none(),
        //     "Tried to reallocate pointer {} ({}) to {} ({}), but it overlaps with an active
        //     allocation with layout ({}).", old_ptr, old_layout, new_ptr, new_layout,
        //     overlapping->allocated_with);
    }
}

void record_legacy_alloc(allocator_id id, usize size, void *ptr) {}

void record_legacy_dealloc(allocator_id id, void *ptr) {}

void record_legacy_realloc(allocator_id id, void *old_ptr, usize new_size, void *new_ptr) {}

} // namespace vixen::heap
