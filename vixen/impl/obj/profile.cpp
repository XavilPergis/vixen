#include "vixen/allocator/profile.hpp"

#include "vixen/allocator/stacktrace.hpp"
#include "vixen/common.hpp"
#include "vixen/stream.hpp"
#include "vixen/traits.hpp"
#include "vixen/types.hpp"

#include <thread>

namespace vixen::heap {

struct AllocationInfo {
    AllocationInfo(rawptr ptr, Layout layout) : base(ptr), allocated_with(layout) {}

    AllocationInfo(Allocator *alloc, const AllocationInfo &other)
        : base(other.base)
        , allocated_with(other.allocated_with)
        , realloc_count(other.realloc_count) {
        if (other.stack_trace) {
            stack_trace = other.stack_trace->clone(alloc);
        }
    }

    rawptr base;
    Layout allocated_with;

    usize realloc_count{0};
    Option<Vector<void *>> stack_trace{};
};

#define BUCKET_SIZE 16384

usize get_bucket_id(rawptr addr) {
    return (usize)addr / BUCKET_SIZE;
}

struct AllocationRange {
    rawptr start, end;
};

struct allocation_bucket {
    allocation_bucket(Allocator *alloc, rawptr start, rawptr end)
        : start(start), end(end), allocations(alloc) {}

    rawptr start, end;
    Vector<AllocationRange> allocations;
};

rawptr range_start(const AllocationRange &range) {
    return range.start;
}

usize bucket_start(const allocation_bucket &bucket) {
    return get_bucket_id(bucket.start);
}

// this is SO BAD
struct AllocationChecker {
    Allocator *alloc;
    Vector<allocation_bucket> buckets;
    Map<rawptr, AllocationInfo> infos;

    AllocationChecker(Allocator *alloc) : alloc(alloc), buckets(alloc), infos(alloc) {}

    void add_range_at(rawptr addr, AllocationRange range) {
        auto bucket_idx
            = binary_search(buckets.begin(), buckets.end(), get_bucket_id(addr), bucket_start);
        if (bucket_idx.is_err()) {
            allocation_bucket bucket_to_add(alloc, addr, util::offset_rawptr(addr, BUCKET_SIZE));
            buckets.shiftInsert(bucket_idx.unwrap_err(), mv(bucket_to_add));
        }

        auto &bucket = buckets[unify_result(mv(bucket_idx))];
        auto range_idx = binary_search(bucket.allocations.begin(),
            bucket.allocations.end(),
            range.start,
            range_start);
        VIXEN_ASSERT_EXT(range_idx.is_err(), "tried to insert already-existent range");
        bucket.allocations.shiftInsert(range_idx.unwrap_err(), range);
    }

    Option<const AllocationInfo &> add(rawptr ptr, AllocationInfo &&info) {
        rawptr start_addr = ptr;
        rawptr end_addr = util::offset_rawptr(start_addr, info.allocated_with.size);
        auto start_bucket_addr = get_bucket_id(start_addr);
        auto end_bucket_addr = get_bucket_id(end_addr);
        AllocationRange range{start_addr, end_addr};

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

    struct RemovalInfo {
        Option<AllocationInfo> info;
        bool is_dealloc_start_misaligned;
        bool is_dealloc_end_misaligned;
    };

    RemovalInfo remove(rawptr addr, Layout layout, Allocator *info_clone_alloc) {
        rawptr start_addr = addr;
        rawptr end_addr = util::offset_rawptr(start_addr, layout.size);
        AllocationRange range{start_addr, end_addr};

        // VIXEN_WARN("removing {}", addr);

        if (auto overlapping_range = get_overlapping_range(addr)) {
            if (range.start == overlapping_range->start && range.end == overlapping_range->end) {
                auto start_bucket_idx = get_bucket_index(range.start);
                auto start_range_idx = get_range_index(*start_bucket_idx, range.start);
                buckets[*start_bucket_idx].allocations.shiftRemove(*start_range_idx);

                auto end_bucket_idx = get_bucket_index(range.end);
                if (start_bucket_idx != end_bucket_idx) {
                    auto end_range_idx = get_range_index(*end_bucket_idx, range.start);
                    buckets[*end_bucket_idx].allocations.shiftRemove(*end_range_idx);
                }

                return {infos.remove(overlapping_range->start), false, false};
            } else {
                return {AllocationInfo{info_clone_alloc, infos[overlapping_range->start]},
                    overlapping_range->start != start_addr,
                    overlapping_range->end != end_addr};
            }
        }

        return {empty_opt, false, false};
    }

    Option<usize> get_bucket_index(rawptr addr) const {
        return binary_search(buckets.begin(), buckets.end(), get_bucket_id(addr), bucket_start)
            .to_ok();
    }

    Option<usize> get_range_index(usize bucket_idx, rawptr addr) const {
        return binary_search(buckets[bucket_idx].allocations.begin(),
            buckets[bucket_idx].allocations.end(),
            addr,
            range_start)
            .to_ok();
    }

    bool remove_range(AllocationRange range) {
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

                start_bucket.allocations.shiftRemove(start_range_index.unwrap_ok());
                end_bucket.allocations.shiftRemove(end_range_index.unwrap_ok());

                return true;
            }
        }

        return false;
    }

    Option<AllocationRange> get_overlapping_range(rawptr addr) const {
        if (auto alloc_range = get_previous_range(addr)) {
            if (alloc_range->start <= addr && alloc_range->end > addr) {
                return alloc_range;
            }
        }
        return empty_opt;
    }

    Option<AllocationRange> get_overlapping_range(AllocationRange range) const {
        auto prev_from_end = get_previous_range(range.end);
        if (prev_from_end.isNone())
            return empty_opt;

        if (range.start < prev_from_end->end && prev_from_end->start < range.end) {
            return prev_from_end;
        } else {
            return empty_opt;
        }
    }

    Option<AllocationRange> get_previous_range(rawptr addr) const {
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

    AllocationInfo &get_info(rawptr ptr) {
        return infos[ptr];
    }

    usize count() const {
        return infos.len();
    }
};

struct AllocatorInfo {
    AllocatorInfo(Allocator *alloc) : listening_queries(alloc), checker(alloc) {}

    Option<String> name;

    usize num_bytes_in_use = 0;
    usize maximum_bytes_in_use = 0;
    usize num_active_allocations = 0;
    usize maximum_active_allocations = 0;

    usize current_transaction_depth = 0;
    bool should_capture_stack_traces = true;

    Vector<QueryId> listening_queries;
    AllocationChecker checker;
};

struct InternalQueryInfo {
    QueryInfo query;
    AllocatorId attached_to;
};

static usize max_allocator_id = 0;
static usize max_query_id = 0;
static Vector<AllocatorId> freed_allocator_names(debug_allocator());
static Vector<QueryId> freed_query_names(debug_allocator());

static Vector<AllocatorInfo> allocator_infos(debug_allocator());
static Vector<InternalQueryInfo> queries(debug_allocator());

QueryId create_memory_performace_query(AllocatorId alloc_id) {
    QueryId id;
    InternalQueryInfo query_info;
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

void delete_memory_performace_query(QueryId id) {
    InternalQueryInfo *query_info = &queries[id.id];
    AllocatorInfo *affected_alloc_info = &allocator_infos[query_info->attached_to.id];

    if (auto idx = affected_alloc_info->listening_queries.firstIndexOf(id)) {
        affected_alloc_info->listening_queries.remove(*idx);
    }

    freed_query_names.push(id);
}

QueryInfo measure_memory_performace_query(QueryId id) {
    QueryInfo info = queries[id.id].query;
    queries[id.id].query = {};
    return info;
}

void register_allocator(Allocator *alloc) {
    if (freed_allocator_names.len() > 0) {
        alloc->id = *freed_allocator_names.pop();
        allocator_infos[alloc->id.id] = AllocatorInfo(debug_allocator());
    } else {
        alloc->id = {static_cast<isize>(max_allocator_id++)};
        allocator_infos.push(AllocatorInfo(debug_allocator()));
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

void unregister_allocator(Allocator *alloc) {
    AllocatorInfo &info = allocator_infos[alloc->id.id];

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

    TranslationCache cache(debug_allocator());

    // if (info.checker.count() > 0) {
    //     VIXEN_INFO("\tActive Allocations:");
    // }
    // info.checker.infos.iter([&](auto &ptr, auto &info) {
    //     VIXEN_WARN("\t\t- Pointer: {}", ptr);
    //     VIXEN_WARN("\t\t\t- layout: {}", info.allocated_with);
    //     VIXEN_WARN("\t\t\t- Realloc Count: {}", info.realloc_count);

    //     if (info.stack_trace) {
    //         vector<AddressInfo> addr_infos = translate_stack_trace(&cache, *info.stack_trace);
    //         print_stack_trace_capture(addr_infos);
    //     }
    // });

    freed_allocator_names.push(alloc->id);
}

usize get_active_allocation_count(AllocatorId id) {
    return allocator_infos[id.id].num_active_allocations;
}

usize get_active_allocation_max_count(AllocatorId id) {
    return allocator_infos[id.id].maximum_active_allocations;
}

usize get_active_byte_count(AllocatorId id) {
    return allocator_infos[id.id].num_bytes_in_use;
}

usize get_active_byte_max_count(AllocatorId id) {
    return allocator_infos[id.id].maximum_bytes_in_use;
}

void set_allocator_name(AllocatorId id, StringView name) {
    AllocatorInfo &info = allocator_infos[id.id];
    if (info.name.isNone()) {
        info.name = String(debug_allocator(), name);
    } else {
        info.name->clear();
        info.name->push(name);
    }
}

Option<StringView> get_allocator_name(AllocatorId id) {
    Option<String> &name = allocator_infos[id.id].name;
    return name ? Option<StringView>{*name} : Option<StringView>{};
}

// allocator_id set_allocator_parent(allocator_id child, allocator_id parent) {}

// allocator_id get_allocator_parent(allocator_id child) {}

void begin_transaction(AllocatorId id) {
    if (id != debug_allocator()->id && id != NOT_TRACKED_ID) {
        allocator_infos[id.id].current_transaction_depth += 1;
    }
}

void end_transaction(AllocatorId id) {
    if (id != debug_allocator()->id && id != NOT_TRACKED_ID) {
        allocator_infos[id.id].current_transaction_depth -= 1;
    }
}

template <typename Stream>
void format_stack_trace(Stream &stream, TranslationCache &cache, const Slice<void *> &stack_trace) {
    auto translated = translate_stack_trace(&cache, stack_trace);
    for (usize i = 0; i < translated.len(); ++i) {
        format_address_info(stream, translated[i], i + 1);
    }
}

static void commit_alloc(AllocatorInfo *alloc_info, Layout layout, void *ptr) {
    alloc_info->num_bytes_in_use += layout.size;
    alloc_info->num_active_allocations += 1;

    alloc_info->maximum_active_allocations
        = std::max(alloc_info->maximum_active_allocations, alloc_info->num_active_allocations);
    alloc_info->maximum_bytes_in_use
        = std::max(alloc_info->maximum_bytes_in_use, alloc_info->num_bytes_in_use);

    for (usize i = 0; i < alloc_info->listening_queries.len(); ++i) {
        InternalQueryInfo *query = &queries[alloc_info->listening_queries[i].id];

        query->query.bytes_in_use += layout.size;
        query->query.cum_allocation_bytes += layout.size;
        query->query.active_allocations += 1;
        query->query.allocation_count += 1;

        query->query.maximum_active_allocations
            = std::max(query->query.maximum_active_allocations, query->query.active_allocations);
        query->query.maximum_bytes_in_use
            = std::max(query->query.maximum_bytes_in_use, query->query.bytes_in_use);
    }

    AllocationInfo info(ptr, layout);
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

static void commit_dealloc(AllocatorInfo *alloc_info, Layout layout, void *ptr) {
    alloc_info->num_bytes_in_use -= layout.size;
    alloc_info->num_active_allocations -= 1;

    for (usize i = 0; i < alloc_info->listening_queries.len(); ++i) {
        InternalQueryInfo *query = &queries[alloc_info->listening_queries[i].id];

        query->query.bytes_in_use -= layout.size;
        query->query.cum_deallocation_bytes += layout.size;
        query->query.active_allocations -= 1;
        query->query.deallocation_count += 1;
    }

    // option<AllocationInfo> prev = alloc_info->checker.infos.remove(ptr);

    // VIXEN_ASSERT_EXT(
    //     prev->allocated_with.size == layout.size && prev->allocated_with.align == layout.align,
    //     "Tried to deallocate pointer at {} using {}, but the pointer was allocated using {}.",
    //     ptr,
    //     layout,
    //     prev->allocated_with);

    auto removal_info = alloc_info->checker.remove(ptr, layout, debug_allocator());
    if (auto &info = removal_info.info) {
        if (removal_info.is_dealloc_start_misaligned || removal_info.is_dealloc_end_misaligned) {
            Vector<char> diagnostic(debug_allocator());
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
                TranslationCache cache(heap::debug_allocator());
                format_stack_trace(bi, cache, *info->stack_trace);
            }

            String diagnostic_str(mv(diagnostic));
            VIXEN_PANIC("{}", diagnostic_str);
        }
    } else {
        VIXEN_PANIC(
            "tried to deallocate pointer at {} using layout {}, but the pointer was not in any active allocation.",
            ptr,
            layout);
    }
}

void record_reset(AllocatorId id) {}

void record_alloc(AllocatorId id, Layout layout, void *ptr) {
    if (debug_allocator()->id == id) {
        return;
    }

    AllocatorInfo *alloc_info = &allocator_infos[id.id];
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

void record_dealloc(AllocatorId id, Layout layout, void *ptr) {
    if (debug_allocator()->id == id) {
        return;
    }

    AllocatorInfo *alloc_info = &allocator_infos[id.id];
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
    AllocatorId id, Layout old_layout, void *old_ptr, Layout new_layout, void *new_ptr) {
    if (debug_allocator()->id == id) {
        return;
    }

    AllocatorInfo *alloc_info = &allocator_infos[id.id];
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
            InternalQueryInfo *query = &queries[alloc_info->listening_queries[i].id];

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
                Vector<char> diagnostic(debug_allocator());
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
                    TranslationCache cache(heap::debug_allocator());
                    format_stack_trace(bi, cache, *info->stack_trace);
                }

                String diagnostic_str(mv(diagnostic));
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

        // option<AllocationInfo> prev = alloc_info->checker.infos.remove(old_ptr);
        // VIXEN_ASSERT_EXT(prev.isSome(),
        //     "Tried to reallocate pointer {}, but it was not an active allocation.",
        //     old_ptr);

        // AllocationInfo info{new_layout, prev->realloc_count + 1};
        // if (alloc_info->should_capture_stack_traces) {
        //     info.stack_trace = capture_stack_trace(debug_allocator());
        // }

        // option<AllocationInfo> overlapping = alloc_info->checker.infos.insert(new_ptr,
        // mv(info)); VIXEN_ASSERT_EXT(overlapping.isNone(),
        //     "Tried to reallocate pointer {} ({}) to {} ({}), but it overlaps with an active
        //     allocation with layout ({}).", old_ptr, old_layout, new_ptr, new_layout,
        //     overlapping->allocated_with);
    }
}

void record_legacy_alloc(AllocatorId id, usize size, void *ptr) {}

void record_legacy_dealloc(AllocatorId id, void *ptr) {}

void record_legacy_realloc(AllocatorId id, void *old_ptr, usize new_size, void *new_ptr) {}

} // namespace vixen::heap
