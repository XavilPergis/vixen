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

template <typename K, typename V, typename H, typename C>
struct hash_map_entry_forward_iterator;

template <typename K, typename V, typename H, typename C>
struct hash_map;

/// @ingroup vixen_data_structures
template <typename K, typename V, typename H, typename C>
struct hash_map_entry {
    constexpr hash_map_entry(hash_map<K, V, H, C> *map_in, usize slot)
        : map_in(map_in), slot(slot) {}

    constexpr hash_map_entry() = default;
    constexpr hash_map_entry(hash_map_entry const &) = default;
    constexpr hash_map_entry &operator=(hash_map_entry const &) = default;

    hash_map<K, V, H, C> *map_in = nullptr;
    option<usize> slot = empty_opt;

    // @NOTE/Flexibility: No mutable key accessor, since it'd be quite easy to accidentally break
    // the invariant that while the key is in the map, neither the hash of the value nor the value
    // itself as it pertains to equality comparisons may change at any point. Note that it IS okay
    // to mutate the key in a way that doesn't affect that though, like updating a cached value
    // that's otherwise transparent.
    constexpr K const &key() const;
    constexpr V const &value() const;
    constexpr V &value();

    /// @brief Removes this entry from the map it's from, and returns the removed key-value pair.
    constexpr tuple<K, V> remove();
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
    hash_map(copy_tag_t, allocator *alloc, const hash_map &other);
    constexpr hash_map(hash_map &&other) = default;
    constexpr hash_map &operator=(hash_map &&other) = default;

    VIXEN_DEFINE_CLONE_METHOD(hash_map)

    using iterator = hash_map_entry_forward_iterator<K, V, Hasher, Cmp>;
    using entry_type = hash_map_entry<K, V, Hasher, Cmp>;

    /// @brief Looks up the value associated with `key` and returns it, or nothing if the entry does
    /// not exist.
    constexpr option<V &> get(K const &key);
    constexpr option<V const &> get(K const &key) const;

    /// @brief Removes an entry with key `key`. If the entry existed, it is returned.
    constexpr option<V> remove(K const &key);

    constexpr option<entry_type> entry(K const &key);

    /// @brief Inserts an entry into the map.
    ///
    /// If an entry with key `key` already existed, then that entry will be evicted and
    /// returned.
    template <typename OK, typename OV>
    option<V> insert(OK &&key, OV &&value);

    template <typename OK, typename... Args>
    V &get_or_insert(OK &&key, Args &&...value);

    constexpr bool contains_key(K const &key) const;

    /// @brief Removes all entries from the map.
    constexpr void clear();

    /// @brief Returns the number of entries in the map.
    // clang-format off
    constexpr usize len() const { return table.len(); }
    // clang-format on

    /// @brief Unchecked access operators.
    constexpr V &operator[](K const &key);
    constexpr V const &operator[](K const &key) const;

    // vector<collision<K>> find_collisions(allocator *alloc) const;
    // hashmap_stats stats() const;

    constexpr iterator begin();
    constexpr iterator end();

    hash_table<tuple<K, V>, Hasher, key_comparator<Cmp, K, V>> table;
};

namespace impl {

template <typename K, typename V, typename H, typename C>
K &get_raw_key(hash_map<K, V, H, C> &map, usize slot) {
    return map.table.get(slot).template get<0>();
}

template <typename K, typename V, typename H, typename C>
V &get_raw_value(hash_map<K, V, H, C> &map, usize slot) {
    return map.table.get(slot).template get<1>();
}

} // namespace impl

} // namespace vixen

#include "vixen/bits/hash/map.inl"
