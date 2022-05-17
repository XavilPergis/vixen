// #include "vixen/allocator/profile.hpp"

// #include "vixen/allocator/stacktrace.hpp"
// #include "vixen/common.hpp"
// #include "vixen/stream.hpp"
// #include "vixen/traits.hpp"
// #include "vixen/types.hpp"

// #include <thread>

// namespace vixen::heap {

// struct AllocationInfo {
//     AllocationInfo(void *ptr, Layout layout) : base(ptr), allocatedWith(layout) {}

//     VIXEN_DEFINE_CLONE_METHOD(AllocationInfo)
//     AllocationInfo(copy_tag_t, Allocator &alloc, const AllocationInfo &other)
//         : base(other.base), allocatedWith(other.allocatedWith), reallocCount(other.reallocCount)
//         { if (other.stackTrace) {
//             stackTrace = cloneOption(alloc, other.stackTrace);
//         }
//     }

//     void *base;
//     Layout allocatedWith;

//     usize reallocCount{0};
//     Option<StackTrace> stackTrace{};
// };

// #define BUCKET_SIZE 16384

// usize getBucketId(void *addr) {
//     return (usize)addr / BUCKET_SIZE;
// }

// struct AllocationRange {
//     void *start, end;
// };

// struct AllocationBucket {
//     AllocationBucket(Allocator &alloc, void *start, void *end)
//         : start(start), end(end), allocations(alloc) {}

//     void *start, end;
//     Vector<AllocationRange> allocations;
// };

// void *range_start(const AllocationRange &range) {
//     return range.start;
// }

// usize bucket_start(const AllocationBucket &bucket) {
//     return getBucketId(bucket.start);
// }

// // this is SO BAD
// struct AllocationChecker {
//     Allocator *alloc;
//     Vector<AllocationBucket> buckets;
//     Map<void * AllocationInfo> infos;

//     AllocationChecker(Allocator &alloc) : alloc(&alloc), buckets(alloc), infos(alloc) {}

//     void add_range_at(void *addr, AllocationRange range) {
//         auto bucketIdx
//             = binary_search(buckets.begin(), buckets.end(), getBucketId(addr), bucket_start);
//         if (bucketIdx.is_err()) {
//             AllocationBucket bucketToAdd(*alloc, addr, util::offsetRaw(addr, BUCKET_SIZE));
//             buckets.insertShift(bucketIdx.unwrap_err(), mv(bucketToAdd));
//         }

//         auto &bucket = buckets[unify_result(mv(bucketIdx))];
//         auto range_idx = binary_search(bucket.allocations.begin(),
//             bucket.allocations.end(),
//             range.start,
//             range_start);
//         VIXEN_ASSERT_EXT(range_idx.is_err(), "tried to insert already-existent range");
//         bucket.allocations.insertShift(range_idx.unwrap_err(), range);
//     }

//     Option<const AllocationInfo &> add(void *ptr, AllocationInfo(&&info)) {
//         void *startAddr = ptr;
//         void *end_addr = util::offsetRaw(startAddr, info.allocatedWith.size);
//         auto startBucketAddr = getBucketId(startAddr);
//         auto endBucketAddr = getBucketId(end_addr);
//         AllocationRange range{startAddr, end_addr};

//         // VIXEN_WARN("adding {}-{} with layout {}", startAddr, end_addr, info.allocatedWith);

//         if (auto overlapping = get_overlapping_range(range)) {
//             // VIXEN_WARN("overlapping {}-{} ", overlapping->start, overlapping->end);
//             return infos[overlapping->start];
//         }

//         add_range_at(startAddr, range);
//         if (startBucketAddr != endBucketAddr) {
//             add_range_at(end_addr, range);
//         }

//         infos.insert(startAddr, mv(info));
//         return EMPTY_OPTION;
//     }

//     struct RemovalInfo {
//         Option<AllocationInfo> info;
//         bool is_dealloc_start_misaligned;
//         bool is_dealloc_end_misaligned;
//     };

//     RemovalInfo remove(void *addr, Layout layout, Allocator &infoCloneAlloc) {
//         void *startAddr = addr;
//         void *end_addr = util::offsetRaw(startAddr, layout.size);
//         AllocationRange range{startAddr, end_addr};

//         // VIXEN_WARN("removing {}", addr);

//         if (auto overlappingRange = get_overlapping_range(addr)) {
//             if (range.start == overlappingRange->start && range.end == overlappingRange->end) {
//                 auto startBucketIdx = get_bucket_index(range.start);
//                 auto startRangeIdx = get_range_index(*startBucketIdx, range.start);
//                 buckets[*startBucketIdx].allocations.removeShift(*startRangeIdx);

//                 auto endBucketIdx = get_bucket_index(range.end);
//                 if (startBucketIdx != endBucketIdx) {
//                     auto endRangeIdx = get_range_index(*endBucketIdx, range.start);
//                     buckets[*endBucketIdx].allocations.removeShift(*endRangeIdx);
//                 }

//                 return RemovalInfo{infos.remove(overlappingRange->start), false, false};
//             } else {
//                 return RemovalInfo{.info = infos[overlappingRange->start].clone(infoCloneAlloc),
//                     .is_dealloc_start_misaligned = overlappingRange->start != startAddr,
//                     .is_dealloc_end_misaligned = overlappingRange->end != end_addr};
//             }
//         }

//         return {EMPTY_OPTION, false, false};
//     }

//     Option<usize> get_bucket_index(void *addr) const {
//         return binary_search(buckets.begin(), buckets.end(), getBucketId(addr), bucket_start)
//             .to_ok();
//     }

//     Option<usize> get_range_index(usize bucketIdx, void *addr) const {
//         return binary_search(buckets[bucketIdx].allocations.begin(),
//             buckets[bucketIdx].allocations.end(),
//             addr,
//             range_start)
//             .to_ok();
//     }

//     bool remove_range(AllocationRange range) {
//         auto start_bucket_index
//             = binary_search(buckets.begin(), buckets.end(), getBucketId(range.start),
//             bucket_start);

//         if (start_bucket_index.is_ok()) {
//             auto &start_bucket = buckets[start_bucket_index.unwrap_ok()];
//             auto start_range_index = binary_search(start_bucket.allocations.begin(),
//                 start_bucket.allocations.end(),
//                 range.start,
//                 range_start);

//             if (start_range_index.is_ok()) {
//                 auto end_bucket_index = binary_search(buckets.begin(),
//                     buckets.end(),
//                     getBucketId(range.end),
//                     bucket_start);
//                 auto &end_bucket = buckets[end_bucket_index.unwrap_ok()];
//                 auto end_range_index = binary_search(end_bucket.allocations.begin(),
//                     end_bucket.allocations.end(),
//                     range.start,
//                     range_start);

//                 if (start_bucket.allocations[start_range_index.unwrap_ok()].end != range.end) {
//                     return false;
//                 }

//                 start_bucket.allocations.removeShift(start_range_index.unwrap_ok());
//                 end_bucket.allocations.removeShift(end_range_index.unwrap_ok());

//                 return true;
//             }
//         }

//         return false;
//     }

//     Option<AllocationRange> get_overlapping_range(void *addr) const {
//         if (auto alloc_range = get_previous_range(addr)) {
//             if (alloc_range->start <= addr && alloc_range->end > addr) {
//                 return alloc_range;
//             }
//         }
//         return EMPTY_OPTION;
//     }

//     Option<AllocationRange> get_overlapping_range(AllocationRange range) const {
//         auto prev_from_end = get_previous_range(range.end);
//         if (prev_from_end.isNone()) return EMPTY_OPTION;

//         if (range.start < prev_from_end->end && prev_from_end->start < range.end) {
//             return prev_from_end;
//         } else {
//             return EMPTY_OPTION;
//         }
//     }

//     Option<AllocationRange> get_previous_range(void *addr) const {
//         auto bucket_addr = getBucketId(addr);
//         if (auto bucketIdx
//             = binary_find_previous_index(buckets.begin(), buckets.end(), bucket_addr,
//             bucket_start))
//         {
//             const auto &bucket = buckets[*bucketIdx];
//             if (auto range_idx = binary_find_previous_index(bucket.allocations.begin(),
//                     bucket.allocations.end(),
//                     addr,
//                     range_start))
//             {
//                 return bucket.allocations[*range_idx];
//             }
//         } else if (bucket_addr > 0) {
//             if (auto bucketIdx = binary_find_previous_index(buckets.begin(),
//                     buckets.end(),
//                     bucket_addr - 1,
//                     bucket_start))
//             {
//                 const auto &bucket = buckets[*bucketIdx];
//                 if (auto range_idx = binary_find_previous_index(bucket.allocations.begin(),
//                         bucket.allocations.end(),
//                         addr,
//                         range_start))
//                 {
//                     return bucket.allocations[*range_idx];
//                 }
//             }
//         }
//         return EMPTY_OPTION;
//     }

//     AllocationInfo &get_info(void *ptr) { return infos[ptr]; }

//     usize count() const { return infos.len(); }
// };

// struct AllocatorInfo {
//     AllocatorInfo(Allocator &alloc) : listeningQueries(alloc), checker(alloc) {}

//     Option<String> name;

//     usize bytesInUse = 0;
//     usize maxBytesInUse = 0;
//     usize activeAllocations = 0;
//     usize maxActiveAllocations = 0;

//     usize currentTransactionDepth = 0;
//     bool shouldCaptureStackTraces = true;

//     Vector<QueryId> listeningQueries;
//     AllocationChecker checker;
// };

// struct InternalQueryInfo {
//     QueryInfo query;
//     AllocatorId attachedTo;
// };

// static usize max_allocator_id = 0;
// static usize max_query_id = 0;
// static Vector<AllocatorId> freed_allocator_names(debugAllocator());
// static Vector<QueryId> freed_query_names(debugAllocator());

// static Vector<AllocatorInfo> allocator_infos(debugAllocator());
// static Vector<InternalQueryInfo> queries(debugAllocator());

// QueryId create_memory_performace_query(AllocatorId alloc_id) {
//     QueryId id;
//     InternalQueryInfo query_info;
//     query_info.query = {};
//     query_info.attachedTo = alloc_id;
//     if (freed_query_names.len() > 0) {
//         id = *freed_query_names.pop();
//         queries[id.id] = query_info;
//     } else {
//         id = {static_cast<isize>(max_query_id++)};
//         queries.insertLast(query_info);
//     }
//     allocator_infos[alloc_id.id].listeningQueries.push(id);
//     return id;
// }

// void delete_memory_performace_query(QueryId id) {
//     InternalQueryInfo *query_info = &queries[id.id];
//     AllocatorInfo *affected_alloc_info = &allocator_infos[query_info->attachedTo.id];

//     if (auto idx = affected_alloc_info->listeningQueries.firstIndexOf(id)) {
//         affected_alloc_info->listeningQueries.remove(*idx);
//     }

//     freed_query_names.insertLast(id);
// }

// QueryInfo measure_memory_performace_query(QueryId id) {
//     QueryInfo info = queries[id.id].query;
//     queries[id.id].query = {};
//     return info;
// }

// void register_allocator(Allocator *alloc) {
//     if (freed_allocator_names.len() > 0) {
//         alloc->id = *freed_allocator_names.pop();
//         allocator_infos[alloc->id.id] = AllocatorInfo(debugAllocator());
//     } else {
//         alloc->id = {static_cast<isize>(max_allocator_id++)};
//         allocator_infos.insertLast(AllocatorInfo(debugAllocator()));
//     }
// }

// constexpr usize ONE_KB = 1024;
// constexpr usize ONE_MB = ONE_KB * 1024;
// constexpr usize ONE_GB = ONE_MB * 1024;
// constexpr usize ONE_TB = ONE_GB * 1024;
// constexpr usize ONE_PB = ONE_TB * 1024;

// static const char *bytes_scale(usize bytes) {
//     // clang-format off
//     if      (bytes < ONE_KB) { return "B";   }
//     else if (bytes < ONE_MB) { return "KiB"; }
//     else if (bytes < ONE_GB) { return "MiB"; }
//     else if (bytes < ONE_TB) { return "GiB"; }
//     else if (bytes < ONE_PB) { return "TiB"; }
//     // clang-format on

//     VIXEN_UNREACHABLE("too many bytes!");
// }

// static usize bytes_units(usize bytes) {
//     // clang-format off
//     if      (bytes < ONE_KB) { return bytes;          }
//     else if (bytes < ONE_MB) { return bytes / ONE_KB; }
//     else if (bytes < ONE_GB) { return bytes / ONE_MB; }
//     else if (bytes < ONE_TB) { return bytes / ONE_GB; }
//     else if (bytes < ONE_PB) { return bytes / ONE_TB; }
//     // clang-format on

//     VIXEN_UNREACHABLE("too many bytes!");
// }

// void unregister_allocator(Allocator *alloc) {
//     AllocatorInfo &info = allocator_infos[alloc->id.id];

//     VIXEN_INFO("allocator stats for `{}`:", info.name ? *info.name : "<unknown>"_s);

//     VIXEN_INFO("\tMaximum tracked bytes used: {} {} ({} bytes)",
//         bytes_units(info.maxBytesInUse),
//         bytes_scale(info.maxBytesInUse),
//         info.maxBytesInUse);

//     VIXEN_INFO("\tCurrent tracked bytes used: {} {} ({} bytes)",
//         bytes_units(info.bytesInUse),
//         bytes_scale(info.bytesInUse),
//         info.bytesInUse);

//     VIXEN_INFO("\tMaximum tracked active allocations: {}", info.maxActiveAllocations);
//     VIXEN_INFO("\tCurrent tracked active allocations: {}", info.activeAllocations);

//     // TranslationCache cache(debugAllocator());

//     // if (info.checker.count() > 0) {
//     //     VIXEN_INFO("\tActive Allocations:");
//     // }
//     // info.checker.infos.iter([&](auto &ptr, auto &info) {
//     //     VIXEN_WARN("\t\t- Pointer: {}", ptr);
//     //     VIXEN_WARN("\t\t\t- layout: {}", info.allocatedWith);
//     //     VIXEN_WARN("\t\t\t- Realloc Count: {}", info.reallocCount);

//     //     if (info.stackTrace) {
//     //         vector<AddressInfo> addr_infos = translate_stack_trace(&cache, *info.stackTrace);
//     //         print_stack_trace_capture(addr_infos);
//     //     }
//     // });

//     freed_allocator_names.insertLast(alloc->id);
// }

// usize get_active_allocation_count(AllocatorId id) {
//     return allocator_infos[id.id].activeAllocations;
// }

// usize get_active_allocation_max_count(AllocatorId id) {
//     return allocator_infos[id.id].maxActiveAllocations;
// }

// usize get_active_byte_count(AllocatorId id) {
//     return allocator_infos[id.id].bytesInUse;
// }

// usize get_active_byte_max_count(AllocatorId id) {
//     return allocator_infos[id.id].maxBytesInUse;
// }

// void set_allocator_name(AllocatorId id, StringView name) {
//     AllocatorInfo &info = allocator_infos[id.id];
//     if (info.name.isNone()) {
//         info.name = String::copyFrom(debugAllocator(), name);
//     } else {
//         info.name->removeAll();
//         info.name->insertLast(name);
//     }
// }

// Option<StringView> get_allocator_name(AllocatorId id) {
//     Option<String> &name = allocator_infos[id.id].name;
//     return name ? Option<StringView>{*name} : Option<StringView>{};
// }

// // allocator_id set_allocator_parent(allocator_id child, allocator_id parent) {}

// // allocator_id get_allocator_parent(allocator_id child) {}

// void begin_transaction(AllocatorId id) {
//     if (id != debugAllocator().id && id != NOT_TRACKED_ID) {
//         allocator_infos[id.id].currentTransactionDepth += 1;
//     }
// }

// void end_transaction(AllocatorId id) {
//     if (id != debugAllocator().id && id != NOT_TRACKED_ID) {
//         allocator_infos[id.id].currentTransactionDepth -= 1;
//     }
// }

// static void commit_alloc(AllocatorInfo *alloc_info, Layout layout, void *ptr) {
//     alloc_info->bytesInUse += layout.size;
//     alloc_info->activeAllocations += 1;

//     alloc_info->maxActiveAllocations
//         = std::max(alloc_info->maxActiveAllocations, alloc_info->activeAllocations);
//     alloc_info->maxBytesInUse = std::max(alloc_info->maxBytesInUse, alloc_info->bytesInUse);

//     for (usize i = 0; i < alloc_info->listeningQueries.len(); ++i) {
//         InternalQueryInfo *query = &queries[alloc_info->listeningQueries[i].id];

//         query->query.bytesInUse += layout.size;
//         query->query.cumAllocationBytes += layout.size;
//         query->query.activeAllocations += 1;
//         query->query.allocationCount += 1;

//         query->query.maxActiveAllocations
//             = std::max(query->query.maxActiveAllocations, query->query.activeAllocations);
//         query->query.maxBytesInUse = std::max(query->query.maxBytesInUse,
//         query->query.bytesInUse);
//     }

//     AllocationInfo info(ptr, layout);
//     if (alloc_info->shouldCaptureStackTraces) {
//         info.stackTrace = StackTrace::captureCurrent(debugAllocator());
//     }

//     if (auto overlapping = alloc_info->checker.add(ptr, mv(info))) {
//         if (alloc_info->name) {
//             VIXEN_PANIC(
//                 "allocation collision: tried to allocate over a previous allocation at {} in
//                 allocator '{}'.\n", overlapping->base, *alloc_info->name);
//         } else {
//             VIXEN_PANIC(
//                 "allocation collision: tried to allocate over a previous allocation at {}.\n",
//                 overlapping->base);
//         }
//         // VIXEN_PANIC(
//         //     "allocation collision: tried to allocate over a previous allocation at {}:\n"
//         //     "previous allocation info:\n"
//         //     "\tat {}\n",
//         //     "\tlayout {}\n",
//         //     overlapping->base,
//         //     overlapping->base,
//         //     overlapping->allocatedWith);
//     }
// }

// static void commit_dealloc(AllocatorInfo *alloc_info, Layout layout, void *ptr) {
//     alloc_info->bytesInUse -= layout.size;
//     alloc_info->activeAllocations -= 1;

//     for (usize i = 0; i < alloc_info->listeningQueries.len(); ++i) {
//         InternalQueryInfo *query = &queries[alloc_info->listeningQueries[i].id];

//         query->query.bytesInUse -= layout.size;
//         query->query.cumDeallocationBytes += layout.size;
//         query->query.activeAllocations -= 1;
//         query->query.deallocationCount += 1;
//     }

//     // option<AllocationInfo> prev = alloc_info->checker.infos.remove(ptr);

//     // VIXEN_ASSERT_EXT(
//     //     prev->allocatedWith.size == layout.size && prev->allocatedWith.align == layout.align,
//     //     "Tried to deallocate pointer at {} using {}, but the pointer was allocated using {}.",
//     //     ptr,
//     //     layout,
//     //     prev->allocatedWith);

//     auto removal_info = alloc_info->checker.remove(ptr, layout, debugAllocator());
//     if (auto &info = removal_info.info) {
//         if (removal_info.is_dealloc_start_misaligned || removal_info.is_dealloc_end_misaligned) {
//             Vector<char> diagnostic(debugAllocator());
//             auto bi = stream::back_inserter(diagnostic);
//             auto oi = stream::make_stream_output_iterator(bi);

//             fmt::format_to(oi, "tried to deallocate, but the deallocation request
//             misaligned:\n");

//             fmt::format_to(oi, "request:\n");
//             if (alloc_info->name) {
//                 fmt::format_to(oi, "- allocator name = '{}'\n", *alloc_info->name);
//             }
//             fmt::format_to(oi, "- pointer = {}\n", ptr);
//             fmt::format_to(oi, "- layout = {}\n", layout);
//             fmt::format_to(oi,
//                 "- start misaligned = {}\n",
//                 removal_info.is_dealloc_start_misaligned);
//             fmt::format_to(oi, "- end misaligned = {}\n",
//             removal_info.is_dealloc_end_misaligned); fmt::format_to(oi, "\n"); fmt::format_to(oi,
//             "block in:\n"); fmt::format_to(oi, "- pointer = {}\n", info->base);
//             fmt::format_to(oi, "- layout = {}\n", info->allocatedWith);
//             fmt::format_to(oi, "- reallocation count = {}\n", info->reallocCount);

//             if (info->stackTrace) {
//                 fmt::format_to(oi, "- stack trace:\n");
//                 formatStackTrace(bi, info->stackTrace.get());
//             }

//             String diagnostic_str(mv(diagnostic));
//             VIXEN_PANIC("{}", diagnostic_str);
//         }
//     } else {
//         VIXEN_PANIC(
//             "tried to deallocate pointer at {} using layout {}, but the pointer was not in any
//             active allocation.", ptr, layout);
//     }
// }

// void record_reset(AllocatorId id) {}

// void record_alloc(AllocatorId id, Layout layout, void *ptr) {
//     if (debugAllocator().id == id) {
//         return;
//     }

//     AllocatorInfo *alloc_info = &allocator_infos[id.id];
//     if (ptr == nullptr || alloc_info->currentTransactionDepth != 1) {
//         return;
//     }

//     if (alloc_info->name) {
//         VIXEN_TRACE("[A] '{}' {} ({})", *alloc_info->name, layout, ptr);
//     } else {
//         VIXEN_TRACE("[A] {} ({})", layout, ptr);
//     }
//     commit_alloc(alloc_info, layout, ptr);
// }

// void record_dealloc(AllocatorId id, Layout layout, void *ptr) {
//     if (debugAllocator().id == id) {
//         return;
//     }

//     AllocatorInfo *alloc_info = &allocator_infos[id.id];
//     if (ptr == nullptr || alloc_info->currentTransactionDepth != 1) {
//         return;
//     }

//     if (alloc_info->name) {
//         VIXEN_TRACE("[D] '{}' {} ({})", *alloc_info->name, layout, ptr);
//     } else {
//         VIXEN_TRACE("[D] {} ({})", layout, ptr);
//     }
//     commit_dealloc(alloc_info, layout, ptr);
// }

// void record_realloc(
//     AllocatorId id, Layout old_layout, void *old_ptr, Layout new_layout, void *new_ptr) {
//     if (debugAllocator().id == id) {
//         return;
//     }

//     AllocatorInfo *alloc_info = &allocator_infos[id.id];
//     if (alloc_info->currentTransactionDepth != 1) {
//         return;
//     }

//     if (old_ptr == nullptr && new_ptr != nullptr) {
//         // Realloc zero -> something, which is an allocation.
//         if (alloc_info->name) {
//             VIXEN_TRACE("[R:A] '{}' {} ({})", *alloc_info->name, new_layout, new_ptr);
//         } else {
//             VIXEN_TRACE("[R:A] {} ({})", new_layout, new_ptr);
//         }
//         commit_alloc(alloc_info, new_layout, new_ptr);
//     } else if (old_ptr != nullptr && new_ptr == nullptr) {
//         // Realloc something -> zero, which is a deallocation.
//         if (alloc_info->name) {
//             VIXEN_TRACE("[R:D] '{}' {} ({})", *alloc_info->name, old_layout, old_ptr);
//         } else {
//             VIXEN_TRACE("[R:D] {} ({})", old_layout, old_ptr);
//         }
//         commit_dealloc(alloc_info, old_layout, old_ptr);
//     } else if (old_ptr != nullptr && new_ptr != nullptr) {
//         // Bona fide reallocation!
//         if (alloc_info->name) {
//             VIXEN_TRACE("[R] '{}' {} ({}) -> {} ({})",
//                 *alloc_info->name,
//                 old_layout,
//                 old_ptr,
//                 new_layout,
//                 old_layout);
//         } else {
//             VIXEN_TRACE("[R] {} ({}) -> {} ({})", old_layout, old_ptr, new_layout, old_layout);
//         }
//         alloc_info->bytesInUse -= old_layout.size;
//         alloc_info->bytesInUse += new_layout.size;
//         alloc_info->maxBytesInUse = std::max(alloc_info->maxBytesInUse, alloc_info->bytesInUse);

//         for (usize i = 0; i < alloc_info->listeningQueries.len(); ++i) {
//             InternalQueryInfo *query = &queries[alloc_info->listeningQueries[i].id];

//             query->query.bytesInUse -= old_layout.size;
//             query->query.bytesInUse += new_layout.size;
//             query->query.cumAllocationBytes += new_layout.size;
//             query->query.cumDeallocationBytes += old_layout.size;
//             query->query.reallocationCount += 1;

//             query->query.maxBytesInUse
//                 = std::max(query->query.maxBytesInUse, query->query.bytesInUse);
//         }

//         auto removal_info = alloc_info->checker.remove(old_ptr, old_layout, debugAllocator());
//         if (auto &info = removal_info.info) {
//             if (removal_info.is_dealloc_start_misaligned ||
//             removal_info.is_dealloc_end_misaligned)
//             {
//                 Vector<char> diagnostic(debugAllocator());
//                 auto bi = stream::back_inserter(diagnostic);
//                 auto oi = stream::make_stream_output_iterator(bi);

//                 fmt::format_to(oi,
//                     "tried to reallocate, but the allocated pointer was in the middle of a
//                     block:\n");

//                 fmt::format_to(oi, "request:\n");
//                 if (alloc_info->name) {
//                     fmt::format_to(oi, "- allocator name = '{}'\n", *alloc_info->name);
//                 }
//                 fmt::format_to(oi, "- old pointer = {}\n", old_ptr);
//                 fmt::format_to(oi, "- old layout = {}\n", old_layout);
//                 fmt::format_to(oi, "- new pointer = {}\n", new_ptr);
//                 fmt::format_to(oi, "- new layout = {}\n", new_layout);
//                 fmt::format_to(oi,
//                     "- start misaligned = {}\n",
//                     removal_info.is_dealloc_start_misaligned);
//                 fmt::format_to(oi,
//                     "- end misaligned = {}\n",
//                     removal_info.is_dealloc_end_misaligned);
//                 fmt::format_to(oi, "\n");
//                 fmt::format_to(oi, "block in:\n");
//                 fmt::format_to(oi, "- pointer = {}\n", info->base);
//                 fmt::format_to(oi, "- layout = {}\n", info->allocatedWith);
//                 fmt::format_to(oi, "- reallocation count = {}\n", info->reallocCount);

//                 if (info->stackTrace) {
//                     fmt::format_to(oi, "- stack trace:\n");
//                     formatStackTrace(bi, *info->stackTrace);
//                 }

//                 String diagnostic_str(mv(diagnostic));
//                 VIXEN_PANIC("{}", diagnostic_str);
//             } else {
//                 // TODO: maybe record stack traces for reallocations too instead of only keeping
//                 // track of the initial alloc.
//                 info->allocatedWith = new_layout;
//                 info->base = new_ptr;
//                 if (auto overlapping = alloc_info->checker.add(new_ptr, mv(*info))) {
//                     VIXEN_PANIC(
//                         "allocation collision: tried to allocate over a previous allocation at
//                         {}.\n", overlapping->base);
//                 }
//             }
//         } else {
//             VIXEN_PANIC(
//                 "tried to reallocate, but the old pointer was not in any active allocation:\n"
//                 "reallocation request:\n",
//                 "old pointer = {}\n"
//                 "old layout = {}\n"
//                 "new pointer = {}\n"
//                 "new layout = {}\n",
//                 old_ptr,
//                 old_layout,
//                 new_ptr,
//                 new_layout);
//         }

//         // option<AllocationInfo> prev = alloc_info->checker.infos.remove(old_ptr);
//         // VIXEN_ASSERT_EXT(prev.isSome(),
//         //     "Tried to reallocate pointer {}, but it was not an active allocation.",
//         //     old_ptr);

//         // AllocationInfo info{new_layout, prev->reallocCount + 1};
//         // if (alloc_info->shouldCaptureStackTraces) {
//         //     info.stackTrace = capture_stack_trace(debugAllocator());
//         // }

//         // option<AllocationInfo> overlapping = alloc_info->checker.infos.insert(new_ptr,
//         // mv(info)); VIXEN_ASSERT_EXT(overlapping.isNone(),
//         //     "Tried to reallocate pointer {} ({}) to {} ({}), but it overlaps with an active
//         //     allocation with layout ({}).", old_ptr, old_layout, new_ptr, new_layout,
//         //     overlapping->allocatedWith);
//     }
// }

// void record_legacy_alloc(AllocatorId id, usize size, void *ptr) {}

// void record_legacy_dealloc(AllocatorId id, void *ptr) {}

// void record_legacy_realloc(AllocatorId id, void *old_ptr, usize new_size, void *new_ptr) {}

// } // namespace vixen::heap
