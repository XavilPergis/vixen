#pragma once

#include "vixen/hash/table.hpp"

namespace vixen {

namespace impl {
constexpr u8 CONTROL_DELETED = 0b10000000;
constexpr u8 CONTROL_FREE = 0b11111111;

constexpr bool isVacant(u8 control) {
    return (control & 0x80) != 0;
}

constexpr bool isFree(u8 control) {
    return control == CONTROL_FREE;
}

constexpr bool controlMatches(u8 control, u8 hash2) {
    // Note that the hash2 comparison also acts as an existence check, since a vacant slot has
    // the highest bit set, and the mask with 0x7f removes the highest bit from the computed
    // hash.
    return (hash2 & 0x7f) == control;
}

constexpr usize extractH1(usize hash) {
    return hash >> 7;
}

constexpr usize extractH2(usize hash) {
    return hash & 0x7f;
}

} // namespace impl

#define _VIXEN_ASSERT_TABLE_ALLOC \
    VIXEN_DEBUG_ASSERT_EXT(alloc != nullptr, "tried to operate on default hash table");

template <typename T, typename H, typename C>
HashTable<T, H, C>::HashTable(Allocator &alloc, usize defaultCapacity) : alloc(alloc) {
    heap::allocParallel<u8, T>(alloc, defaultCapacity, control, buckets);
    util::fillUninitialized(impl::CONTROL_FREE, control, defaultCapacity);
    capacity = defaultCapacity;
}

template <typename T, typename H, typename C>
HashTable<T, H, C>::HashTable(copy_tag_t, Allocator &alloc, const HashTable &other)
    : HashTable(alloc, other.capacity) {
    for (usize i = 0; i < capacity; ++i) {
        if (!impl::isVacant(other.control[i])) {
            auto const &entry = C::map_entry(other.buckets[i]);
            auto hash = makeHash<H>(entry);
            usize idx = findInsertSlot(hash, entry);
            insertNoResize(idx, hash, copyAllocate(alloc, other.buckets[i]));
        }
    }

    util::arrayCopyNonoverlapping(other.control, control, capacity);
}

template <typename T, typename H, typename C>
constexpr HashTable<T, H, C>::HashTable(HashTable &&other) noexcept
    : alloc(std::exchange(other.alloc, nullptr))
    , capacity(std::exchange(other.capacity, 0))
    , occupied(std::exchange(other.occupied, 0))
    , items(std::exchange(other.items, 0))
    , control(std::exchange(other.control, nullptr))
    , buckets(std::exchange(other.buckets, nullptr)) {}

template <typename T, typename H, typename C>
constexpr HashTable<T, H, C> &HashTable<T, H, C>::operator=(HashTable<T, H, C> &&other) noexcept {
    if (std::addressof(other) == this) return *this;

    alloc = std::exchange(other.alloc, nullptr);

    capacity = std::exchange(other.capacity, 0);
    occupied = std::exchange(other.occupied, 0);
    items = std::exchange(other.items, 0);

    control = std::exchange(other.control, nullptr);
    buckets = std::exchange(other.buckets, nullptr);

    return *this;
}

template <typename T, typename H, typename C>
HashTable<T, H, C>::~HashTable() {
    if (alloc == nullptr) return;

    // not calling removeAll means we won't reset all of the control bytes, but that's okay since
    // we're not doing anything else with this table afterwards.
    if constexpr (!std::is_trivial_v<T>) {
        removeAll();
    }

    if (alloc && capacity > 0) {
        heap::deallocParallel<u8, T>(*alloc, capacity, control, buckets);
    }
}

template <typename T, typename H, typename C>
void HashTable<T, H, C>::grow() {
    _VIXEN_ASSERT_TABLE_ALLOC
    if (doesTableNeedResize()) {
        usize newCap = capacity == 0 ? DEFAULT_HASHMAP_CAPACITY : capacity * 2;
        HashTable newTable(alloc, newCap);

        for (usize i = 0; i < capacity; ++i) {
            if (!impl::isVacant(control[i])) {
                auto const &entry = C::map_entry(buckets[i]);
                auto hash = makeHash<H>(entry);
                auto slot = newTable.findInsertSlot(hash, entry);
                newTable.insertNoResize(slot, hash, mv(buckets[i]));
            }
        }

        std::swap(*this, newTable);
    }
}

template <typename T, typename H, typename C>
void HashTable<T, H, C>::insert(usize slot, u64 hash, T &&value) {
    _VIXEN_ASSERT_TABLE_ALLOC
    insertNoResize(slot, hash, mv(value));
}

// Doesn't check if the table needs resizing
// Doesn't destruct old value
template <typename T, typename H, typename C>
constexpr void HashTable<T, H, C>::insertNoResize(usize slot, u64 hash, T &&value) noexcept {
    _VIXEN_ASSERT_TABLE_ALLOC
    new (&buckets[slot]) T{mv(value)};

    items += impl::isVacant(control[slot]);
    occupied += impl::isFree(control[slot]);
    control[slot] = impl::extractH2(hash) & 0x7f;
}

template <typename T, typename H, typename C>
constexpr void HashTable<T, H, C>::remove(usize slot) noexcept {
    _VIXEN_ASSERT_TABLE_ALLOC
    items -= 1;
    bool isNextFree = impl::isFree(control[(slot + 1) % capacity]);
    occupied -= isNextFree;
    control[slot] = isNextFree ? impl::CONTROL_FREE : impl::CONTROL_DELETED;
    buckets[slot].~T();
}

template <typename T, typename H, typename C>
constexpr T &HashTable<T, H, C>::get(usize slot) noexcept {
    _VIXEN_ASSERT_TABLE_ALLOC
    return buckets[slot];
}

template <typename T, typename H, typename C>
constexpr const T &HashTable<T, H, C>::get(usize slot) const noexcept {
    _VIXEN_ASSERT_TABLE_ALLOC
    return buckets[slot];
}

template <typename T, typename H, typename C>
constexpr bool HashTable<T, H, C>::isOccupied(usize slot) const noexcept {
    _VIXEN_ASSERT_TABLE_ALLOC
    return !impl::isVacant(control[slot]);
}

template <typename T, typename H, typename C>
constexpr void HashTable<T, H, C>::removeAll() noexcept {
    _VIXEN_ASSERT_TABLE_ALLOC
    for (usize i = 0; i < capacity; ++i) {
        if (!impl::isVacant(control[i])) {
            buckets[i].~T();
            control[i] = impl::CONTROL_FREE;
        }
    }
}

template <typename T, typename H, typename C>
template <typename OT>
constexpr Option<usize> HashTable<T, H, C>::findSlot(u64 hash, const OT &value) const noexcept {
    _VIXEN_ASSERT_TABLE_ALLOC
    if (capacity == 0) return EMPTY_OPTION;

    u64 hash1 = impl::extractH1(hash) % capacity;
    u8 hash2 = impl::extractH2(hash);

    for (usize i = hash1;; i = (i + 1) % capacity) {
        if (impl::isFree(control[i])) {
            return EMPTY_OPTION;
        }

        if (impl::controlMatches(control[i], hash2) && likely(C::eq(buckets[i], value))) {
            return i;
        }
    }

    VIXEN_UNREACHABLE();
}

template <typename T, typename H, typename C>
template <typename OT>
constexpr usize HashTable<T, H, C>::findInsertSlot(u64 hash, const OT &value) const noexcept {
    _VIXEN_ASSERT_TABLE_ALLOC
    VIXEN_DEBUG_ASSERT(capacity > 0);

    u64 hash1 = impl::extractH1(hash) % capacity;
    u8 hash2 = impl::extractH2(hash);

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
        if (impl::isVacant(control[i])) {
            return i;
        }

        if (impl::controlMatches(control[i], hash2) && likely(C::eq(buckets[i], value))) {
            return i;
        }
    }

    VIXEN_UNREACHABLE();
}

template <typename T, typename H, typename C>
constexpr bool HashTable<T, H, C>::doesTableNeedResize() const noexcept {
    _VIXEN_ASSERT_TABLE_ALLOC
    // integer-only check for `occupied / capacity >= 0.7`
    return LOAD_FACTOR_DENOMENATOR * occupied >= LOAD_FACTOR_NUMERATOR * capacity;
}

} // namespace vixen