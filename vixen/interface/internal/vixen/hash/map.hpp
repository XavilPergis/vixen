#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/hash/table.hpp"
#include "vixen/tuple.hpp"

namespace vixen {

template <typename Cmp, typename K, typename V>
struct key_comparator {
    template <typename U>
    constexpr static bool eq(const tuple<K, V> &lhs, const U &rhs) {
        return Cmp::eq(lhs.template get<0>(), rhs);
    }

    constexpr static const K &map_entry(const tuple<K, V> &entry) {
        return entry.template get<0>();
    }
};

/// @ingroup vixen_data_structures
template <typename K,
    typename V,
    typename Hasher = default_hasher,
    typename Cmp = default_comparator<K>>
struct hash_map {
    constexpr hash_map() = default;

    constexpr explicit hash_map(allocator *alloc);
    hash_map(allocator *alloc, usize default_capacity);
    hash_map(allocator *alloc, const hash_map &other);
    constexpr hash_map(hash_map &&other) = default;
    constexpr hash_map &operator=(hash_map &&other) = default;

    VIXEN_DEFINE_CLONE_METHOD(hash_map)

    /// Looks up the value associated with `key` and returns it, or nothing if the entry does not
    /// exist.
    constexpr option<V &> get(K const &key);
    constexpr option<V const &> get(K const &key) const;

    /// @brief Removes an entry with key `key`. If the entry existed, it is returned.
    constexpr option<V> remove(K const &key);

    /// @brief Inserts an entry into the map.
    ///
    /// If an entry with key `key` already existed, then that entry will be evicted and returned.
    template <typename OK, typename OV>
    option<V> insert(OK &&key, OV &&value);

    constexpr bool key_exists(K const &key) const;

    /// @brief Removes all entries from the map.
    constexpr void clear();

    /// @brief Returns the number of entries in the map.
    // clang-format off
    constexpr usize len() const { return table.len(); }
    // clang-format on

    constexpr V &operator[](K const &key);
    constexpr V const &operator[](K const &key) const;

    // vector<collision<K>> find_collisions(allocator *alloc) const;
    // hashmap_stats stats() const;

    hash_table<tuple<K, V>, Hasher, key_comparator<Cmp, K, V>> table;
};

} // namespace vixen

#include "vixen/bits/hash/map.inl"
