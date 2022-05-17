#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/hash/table.hpp"
#include "vixen/tuple.hpp"

namespace vixen {

template <typename Cmp, typename K, typename V>
struct key_comparator {
    template <typename U>
    constexpr static bool eq(const Tuple<K, V> &lhs, const U &rhs) {
        return Cmp::eq(lhs.template get<0>(), rhs);
    }

    constexpr static const K &map_entry(const Tuple<K, V> &entry) {
        return entry.template get<0>();
    }
};

template <typename K, typename V, typename H, typename C>
struct HashMapEntryForwardIterator;

template <typename K, typename V, typename H, typename C>
struct HashMap;

/// @ingroup vixen_data_structures
template <typename K, typename V, typename H, typename C>
struct HashMapEntry {
    constexpr HashMapEntry(HashMap<K, V, H, C> *mapIn, usize slot) noexcept
        : mapIn(mapIn), slot(slot) {}

    constexpr HashMapEntry() = default;
    constexpr HashMapEntry(HashMapEntry const &) = default;
    constexpr HashMapEntry &operator=(HashMapEntry const &) = default;

    HashMap<K, V, H, C> *mapIn = nullptr;
    Option<usize> slot = EMPTY_OPTION;

    // @NOTE/Flexibility: No mutable key accessor, since it'd be quite easy to accidentally break
    // the invariant that while the key is in the map, neither the hash of the value nor the value
    // itself as it pertains to equality comparisons may change at any point. Note that it IS okay
    // to mutate the key in a way that doesn't affect that though, like updating a cached value
    // that's otherwise transparent.
    constexpr K const &key() const noexcept;
    constexpr V const &value() const noexcept;
    constexpr V &value() noexcept;

    /// @brief Removes this entry from the map it's from, and returns the removed key-value pair.
    constexpr Tuple<K, V> remove() noexcept;
};

/// @ingroup vixen_data_structures
template <typename K,
    typename V,
    typename Hasher = default_hasher,
    typename Cmp = default_comparator<K>>
struct HashMap {
    constexpr HashMap() = default;
    VIXEN_MOVE_ONLY(HashMap)

    constexpr explicit HashMap(Allocator &alloc) noexcept;
    HashMap(Allocator &alloc, usize defaultCapacity);
    HashMap(copy_tag_t, Allocator &alloc, const HashMap &other);
    VIXEN_DEFINE_CLONE_METHOD(HashMap)

    using iterator = HashMapEntryForwardIterator<K, V, Hasher, Cmp>;
    using entry_type = HashMapEntry<K, V, Hasher, Cmp>;

    /// @brief Looks up the value associated with `key` and returns it, or nothing if the entry does
    /// not exist.
    constexpr Option<V &> get(K const &key) noexcept;
    constexpr Option<V const &> get(K const &key) const noexcept;

    /// @brief Removes an entry with key `key`. If the entry existed, it is returned.
    constexpr Option<V> remove(K const &key) noexcept;

    constexpr Option<entry_type> entry(K const &key) noexcept;

    /// @brief Inserts an entry into the map.
    ///
    /// If an entry with key `key` already existed, then that entry will be evicted and
    /// returned.
    template <typename OK, typename OV>
    Option<V> insert(OK &&key, OV &&value);

    template <typename OK, typename... Args>
    V &getOrInsert(OK &&key, Args &&...value);

    constexpr bool containsKey(K const &key) const noexcept;

    /// @brief Removes all entries from the map.
    constexpr void removeAll();

    /// @brief Returns the number of entries in the map.
    // clang-format off
    constexpr usize len() const noexcept { return table.len(); }
    constexpr usize capacity() const noexcept { return table.capacity(); }
    // clang-format on

    /// @brief Unchecked access operators.
    constexpr V &operator[](K const &key) noexcept;
    constexpr V const &operator[](K const &key) const noexcept;

    // vector<collision<K>> find_collisions(Allocator &alloc) const;
    // hashmap_stats stats() const;

    constexpr iterator begin() noexcept;
    constexpr iterator end() noexcept;

    HashTable<Tuple<K, V>, Hasher, key_comparator<Cmp, K, V>> table;
};

namespace impl {

template <typename K, typename V, typename H, typename C>
K &getRawKey(HashMap<K, V, H, C> &map, usize slot) noexcept {
    return map.table.get(slot).template get<0>();
}

template <typename K, typename V, typename H, typename C>
V &getRawValue(HashMap<K, V, H, C> &map, usize slot) noexcept {
    return map.table.get(slot).template get<1>();
}

} // namespace impl

template <typename K,
    typename V,
    typename Hasher = default_hasher,
    typename Cmp = default_comparator<K>>
using Map = HashMap<K, V, Hasher, Cmp>;

} // namespace vixen

#include "vixen/bits/hash/map.inl"
