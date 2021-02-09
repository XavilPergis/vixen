#pragma once

#include "vixen/hash/table.hpp"

namespace vixen {

namespace impl {
constexpr u8 control_deleted = 0b10000000;
constexpr u8 control_free = 0b11111111;

constexpr bool is_vacant(u8 control) {
    return (control & 0x80) != 0;
}

constexpr bool is_free(u8 control) {
    return control == control_free;
}

constexpr bool control_matches(u8 control, u8 hash2) {
    // Note that the hash2 comparison also acts as an existence check, since a vacant slot has
    // the highest bit set, and the mask with 0x7f removes the highest bit from the computed
    // hash.
    return (hash2 & 0x7f) == control;
}

constexpr usize extract_h1(usize hash) {
    return hash >> 7;
}

constexpr usize extract_h2(usize hash) {
    return hash & 0x7f;
}

} // namespace impl

template <typename T, typename H, typename C>
hash_table<T, H, C>::hash_table(allocator *alloc, usize default_capacity) : alloc(alloc) {
    heap::alloc_parallel<u8, T>(alloc, default_capacity, control, buckets);
    util::fill(impl::control_free, control, default_capacity);
    capacity = default_capacity;
}

template <typename T, typename H, typename C>
hash_table<T, H, C>::hash_table(allocator *alloc, const hash_table &other)
    : hash_table(alloc, other.capacity) {
    for (usize i = 0; i < capacity; ++i) {
        if (!impl::is_vacant(other.control[i])) {
            auto const &entry = C::map_entry(other.buckets[i]);
            auto hash = make_hash<H>(entry);
            usize idx = find_insert_slot(hash, entry);
            insert_no_resize(idx,
                hash,
                copy_construct_maybe_allocator_aware(alloc, other.buckets[i]));
        }
    }

    util::copy_nonoverlapping(other.control, control, capacity);
}

template <typename T, typename H, typename C>
constexpr hash_table<T, H, C>::hash_table(hash_table &&other)
    : alloc(std::exchange(other.alloc, nullptr))
    , capacity(std::exchange(other.capacity, 0))
    , occupied(std::exchange(other.occupied, 0))
    , items(std::exchange(other.items, 0))
    , control(std::exchange(other.control, nullptr))
    , buckets(std::exchange(other.buckets, nullptr)) {}

template <typename T, typename H, typename C>
constexpr hash_table<T, H, C> &hash_table<T, H, C>::operator=(hash_table<T, H, C> &&other) {
    if (std::addressof(other) == this)
        return *this;

    alloc = std::exchange(other.alloc, nullptr);

    capacity = std::exchange(other.capacity, 0);
    occupied = std::exchange(other.occupied, 0);
    items = std::exchange(other.items, 0);

    control = std::exchange(other.control, nullptr);
    buckets = std::exchange(other.buckets, nullptr);

    return *this;
}

template <typename T, typename H, typename C>
hash_table<T, H, C>::~hash_table() {
    // not calling clear means we won't reset all of the control bytes, but that's okay since we're
    // not doing anything else with this table afterwards.
    if constexpr (!std::is_trivial_v<T>) {
        clear();
    }

    if (alloc && capacity > 0) {
        heap::dealloc_parallel<u8, T>(alloc, capacity, control, buckets);
    }
}

template <typename T, typename H, typename C>
void hash_table<T, H, C>::grow() {
    if (does_table_need_resize()) {
        usize new_cap = capacity == 0 ? default_hashmap_capacity : capacity * 2;
        hash_table new_table(alloc, new_cap);

        for (usize i = 0; i < capacity; ++i) {
            if (!impl::is_vacant(control[i])) {
                auto const &entry = C::map_entry(buckets[i]);
                auto hash = make_hash<H>(entry);
                auto slot = new_table.find_insert_slot(hash, entry);
                new_table.insert_no_resize(slot, hash, mv(buckets[i]));
            }
        }

        std::swap(*this, new_table);
    }
}

template <typename T, typename H, typename C>
void hash_table<T, H, C>::insert(usize slot, u64 hash, T &&value) {
    insert_no_resize(slot, hash, mv(value));
}

// Doesn't check if the table needs resizing
// Doesn't destruct old value
template <typename T, typename H, typename C>
constexpr void hash_table<T, H, C>::insert_no_resize(usize slot, u64 hash, T &&value) {
    util::construct_in_place(&buckets[slot], mv(value));

    items += impl::is_vacant(control[slot]);
    occupied += impl::is_free(control[slot]);
    control[slot] = impl::extract_h2(hash) & 0x7f;
}

template <typename T, typename H, typename C>
constexpr void hash_table<T, H, C>::remove(usize slot) {
    items -= 1;
    bool is_next_free = impl::is_free(control[(slot + 1) % capacity]);
    occupied -= is_next_free;
    control[slot] = is_next_free ? impl::control_free : impl::control_deleted;
    buckets[slot].~T();
}

template <typename T, typename H, typename C>
constexpr T &hash_table<T, H, C>::get(usize slot) {
    return buckets[slot];
}

template <typename T, typename H, typename C>
constexpr const T &hash_table<T, H, C>::get(usize slot) const {
    return buckets[slot];
}

template <typename T, typename H, typename C>
constexpr bool hash_table<T, H, C>::is_occupied(usize slot) const {
    return !impl::is_vacant(control[slot]);
}

template <typename T, typename H, typename C>
constexpr void hash_table<T, H, C>::clear() {
    for (usize i = 0; i < capacity; ++i) {
        if (!impl::is_vacant(control[i])) {
            buckets[i].~T();
            control[i] = impl::control_free;
        }
    }
}

template <typename T, typename H, typename C>
template <typename OT>
constexpr option<usize> hash_table<T, H, C>::find_slot(u64 hash, const OT &value) const {
    u64 hash1 = impl::extract_h1(hash) % capacity;
    u8 hash2 = impl::extract_h2(hash);

    for (usize i = hash1;; i = (i + 1) % capacity) {
        if (impl::is_free(control[i])) {
            return empty_opt;
        }

        if (impl::control_matches(control[i], hash2) && likely(C::eq(buckets[i], value))) {
            return i;
        }
    }

    VIXEN_UNREACHABLE();
}

template <typename T, typename H, typename C>
template <typename OT>
constexpr usize hash_table<T, H, C>::find_insert_slot(u64 hash, const OT &value) const {
    VIXEN_DEBUG_ASSERT(capacity > 0);

    u64 hash1 = impl::extract_h1(hash) % capacity;
    u8 hash2 = impl::extract_h2(hash);

    // We use linear probing here for the following reason:
    //
    // Instead of dealing with hash collisions by maintaining a linked list at each bucket
    // (confusingly deemed open *hashing*), we "slip" collided values into another slot. You can
    // sort of think as this as an implicit linked list, where the "arrows" to the next item are
    // based on the probing function used. This implicit list is a "probe chain". Getting the slot
    // index of a given value means getting the start of the probe chain and walking it until we
    // either find a matching value, *or encounter a free slot*.
    //
    // This is all fine and well for handling insertions, but when we want to *delete* an item, we
    // can't just set its slot to free, because that would cause a break in the probe chain. Items
    // after the now free slot would become inaccessible, and we could end up in all sorts of
    // funkyness like having two identical values in the same table.
    //
    // To solve this, we use sentinal slots dubbed "tombstones" or "deleted" slots that act like
    // free slots in every way except that they don't break the probe chain.
    for (usize i = hash1;; i = (i + 1) % capacity) {
        if (impl::is_vacant(control[i])) {
            return i;
        }

        if (impl::control_matches(control[i], hash2) && likely(C::eq(buckets[i], value))) {
            return i;
        }
    }

    VIXEN_UNREACHABLE();
}

template <typename T, typename H, typename C>
constexpr bool hash_table<T, H, C>::does_table_need_resize() const {
    // integer-only check for `occupied / capacity >= 0.7`
    return load_factor_denomenator * occupied >= load_factor_numerator * capacity;
}

} // namespace vixen