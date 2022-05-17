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

// struct Allocator;

// struct AllocatorId {
//     isize id;

//     bool operator==(const AllocatorId &other) const { return id == other.id; }
//     bool operator!=(const AllocatorId &other) const { return id != other.id; }
// };

// struct QueryId {
//     isize id;

//     bool operator==(const QueryId &other) const { return id == other.id; }
//     bool operator!=(const QueryId &other) const { return id != other.id; }
// };

// struct AllocatorEvent {
//     virtual ~AllocatorEvent() {}
// };

// struct ResetEvent : AllocatorEvent {};

// struct AllocEvent : AllocatorEvent {
//     Layout layout;
//     void *ptr;
// };

// struct DeallocEvent : AllocatorEvent {
//     Layout layout;
//     void *ptr;
// };

// struct ReallocEvent : AllocatorEvent {
//     Layout oldLayout, newLayout;
//     void *oldPtr, *newPtr;
// };

// struct LegacyAllocEvent : AllocatorEvent {
//     usize size;
//     void *ptr;
// };

// struct LegacyDeallocEvent : AllocatorEvent {
//     void *ptr;
// };

// struct LegacyReallocEvent : AllocatorEvent {
//     usize newSize;
//     void *oldPtr, *newPtr;
// };

// struct AllocatorLayer {
//     virtual ~AllocatorLayer() {}

//     virtual void beginTransaction() = 0;
//     virtual void endTransaction() = 0;

//     virtual void record(AllocatorEvent event) = 0;

//     virtual void setName(StringView name);
//     virtual Option<StringView> getName();

//     // QueryId create_memory_performace_query(AllocatorId id);
//     // QueryInfo measure_memory_performace_query(QueryId id);
//     // void delete_memory_performace_query(QueryId id);

//     usize active_allocation_count(AllocatorId id);
//     usize active_allocation_max_count(AllocatorId id);
//     usize active_byte_count(AllocatorId id);
//     usize active_byte_max_count(AllocatorId id);

// private:
//     Option<Unique<AllocatorLayer>> mNext;
// };

// template <typename T>
// struct Producer {
//     virtual ~Producer() {}
//     virtual T operator()() = 0;
// };

// template <typename T, typename F>
// struct TypedProducer {
//     TypedProducer(F &&producer) : producer(std::forward<F>(producer)) {}

//     virtual void operator()() override { producer(); }

//     F producer;
// };

// /**
//  * @brief Adds an allocator layer that will be applied to all allocators created after this
//  function
//  * call completes.
//  *
//  * @param layer
//  */
// void addDefaultAllocatorLayer(Producer<AllocatorLayer> *layerFactory);

// /**
//  * @brief Adds an allocator layer to the specified allocator.
//  *
//  * @param alloc
//  * @param layer
//  */
// void addAllocatorLayer(Allocator &alloc, AllocatorLayer &layer);

// struct AllocatorProfiler {
//     virtual ~AllocatorProfiler();

//     void beginTracking(Allocator *alloc);
//     void endTracking(Allocator *alloc);

//     void set_allocator_name(AllocatorId id, StringView name);
//     Option<StringView> get_allocator_name(AllocatorId id);

//     // allocator_id set_allocator_parent(allocator_id child, allocator_id parent);
//     // allocator_id get_allocator_parent(allocator_id child);

//     QueryId create_memory_performace_query(AllocatorId id);
//     QueryInfo measure_memory_performace_query(QueryId id);
//     void delete_memory_performace_query(QueryId id);

//     usize get_active_allocation_count(AllocatorId id);
//     usize get_active_allocation_max_count(AllocatorId id);
//     usize get_active_byte_count(AllocatorId id);
//     usize get_active_byte_max_count(AllocatorId id);
// };

// struct QueryInfo {
//     usize bytesInUse = 0;
//     usize activeAllocations = 0;
//     usize maxBytesInUse = 0;
//     usize maxActiveAllocations = 0;

//     usize allocationCount = 0;
//     usize deallocationCount = 0;
//     usize reallocationCount = 0;

//     usize cumAllocationBytes = 0;
//     usize cumDeallocationBytes = 0;
// };

// constexpr AllocatorId NOT_TRACKED_ID = {-1};

// void register_allocator(Allocator *alloc);
// void unregister_allocator(Allocator *alloc);

// void set_allocator_name(AllocatorId id, StringView name);
// Option<StringView> get_allocator_name(AllocatorId id);

// allocator_id set_allocator_parent(allocator_id child, allocator_id parent);
// allocator_id get_allocator_parent(allocator_id child);

// void begin_transaction(AllocatorId id);
// void end_transaction(AllocatorId id);

// void record_reset(AllocatorId id);
// void record_alloc(AllocatorId id, Layout layout, void *ptr);
// void record_dealloc(AllocatorId id, Layout layout, void *ptr);
// void record_realloc(AllocatorId id, Layout oldLayout, void *oldPtr, Layout newLayout, void
// *newPtr);

// void record_legacy_alloc(AllocatorId id, usize size, void *ptr);
// void record_legacy_dealloc(AllocatorId id, void *ptr);
// void record_legacy_realloc(AllocatorId id, void *oldPtr, usize newSize, void *newPtr);

// QueryId create_memory_performace_query(AllocatorId id);
// QueryInfo measure_memory_performace_query(QueryId id);
// void delete_memory_performace_query(QueryId id);

// usize get_active_allocation_count(AllocatorId id);
// usize get_active_allocation_max_count(AllocatorId id);
// usize get_active_byte_count(AllocatorId id);
// usize get_active_byte_max_count(AllocatorId id);

} // namespace vixen::heap
