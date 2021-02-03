#pragma once

#include "vixen/hash/map.hpp"

namespace vixen {

template <typename K, typename V, typename H, typename C>
constexpr hash_map<K, V, H, C>::hash_map(allocator *alloc) : table(alloc) {}

template <typename K, typename V, typename H, typename C>
hash_map<K, V, H, C>::hash_map(allocator *alloc, usize default_capacity)
    : table(alloc, default_capacity) {}

template <typename K, typename V, typename H, typename C>
hash_map<K, V, H, C>::hash_map(allocator *alloc, const hash_map &other)
    : table(alloc, other.table) {}

template <typename K, typename V, typename H, typename C>
constexpr option<V &> hash_map<K, V, H, C>::get(const K &key) {
    if (auto slot_opt = table.find_slot(make_hash<H>(key), key)) {
        return table.get(*slot_opt);
    }
    return nullptr;
}

template <typename K, typename V, typename H, typename C>
constexpr option<V const &> hash_map<K, V, H, C>::get(const K &key) const {
    if (auto slot_opt = table.find_slot(make_hash<H>(key), key)) {
        return table.get(*slot_opt);
    }
    return nullptr;
}

template <typename K, typename V, typename H, typename C>
constexpr option<V> hash_map<K, V, H, C>::remove(const K &key) {
    if (auto slot = table.find_slot(make_hash<H>(key), key)) {
        auto old_entry = mv(table.get(*slot));
        table.remove(*slot);
        return mv(old_entry.template get<1>());
    }
    return nullptr;
}

template <typename K, typename V, typename H, typename C>
template <typename OK, typename OV>
option<V> hash_map<K, V, H, C>::insert(OK &&key, OV &&value) {
    auto hash = make_hash<H>(key);
    auto slot = table.find_insert_slot(hash, key);

    option<V> old;
    if (table.is_occupied(slot)) {
        auto old_entry = mv(table.get(slot));
        old = mv(old_entry.template get<1>());
    }

    table.insert(slot, hash, tuple<K, V>{std::forward<OK>(key), std::forward<OV>(value)});
    return old;
}

template <typename K, typename V, typename H, typename C>
constexpr bool hash_map<K, V, H, C>::key_exists(const K &key) const {
    return (bool)table.find_slot(make_hash<H>(key), key);
}

template <typename K, typename V, typename H, typename C>
constexpr void hash_map<K, V, H, C>::clear() {
    table.clear();
}

template <typename K, typename V, typename H, typename C>
constexpr V &hash_map<K, V, H, C>::operator[](K const &key) {
    auto slot = table.find_slot(make_hash<H>(key), key);
    VIXEN_DEBUG_ASSERT(slot.is_some(), "Tried to access item in hashmap that does not exist.");
    return table.get(*slot).template get<1>();
}

template <typename K, typename V, typename H, typename C>
constexpr V const &hash_map<K, V, H, C>::operator[](K const &key) const {
    auto slot = table.find_slot(make_hash<H>(key), key);
    VIXEN_DEBUG_ASSERT(slot.is_some(), "Tried to access item in hashmap that does not exist.");
    return table.get(*slot).template get<1>();
}

// template <typename K, typename V, typename Hasher = default_hasher>
// struct hash_map {
//     /// The index operators will not do correctness checks!
//     V &operator[](K const &key);
//     V const &operator[](K const &key) const;

//     vector<collision<K>> find_collisions(allocator *alloc) const;
//     hashmap_stats stats() const;

// private:
//     hash_table<tuple<K, V>, Hasher> table;
// };

} // namespace vixen
