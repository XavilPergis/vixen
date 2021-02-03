// #pragma once

// #include "vixen/allocator/allocator.hpp"
// #include "vixen/option.hpp"
// #include "vixen/types.hpp"
// #include "vixen/vec.hpp"

// namespace vixen {

// // Number of slots in the initial allocation of a hashmap.
// constexpr usize default_hashmap_capacity = 32;

// // The ratio of occupied slots to free slots needed before a table resize
// constexpr f64 hashmap_load_factor = 0.7;

// constexpr u8 ctrl_deleted = 0b10000000;
// constexpr u8 ctrl_free = 0b11111111;

// /// @ingroup vixen_data_structures
// template <typename K, typename V, typename Hasher = default_hasher>
// struct hash_map {
//     hash_map();

//     explicit hash_map(allocator *alloc);
//     hash_map(allocator *alloc, usize default_capacity);
//     template <typename OtherHasher>
//     hash_map(allocator *alloc, const hash_map<K, V, OtherHasher> &other);
//     hash_map(hash_map<K, V, Hasher> &&other);
//     hash_map<K, V, Hasher> &operator=(hash_map<K, V, Hasher> &&other);

//     hash_map<K, V, Hasher> clone(allocator *alloc) const;

//     ~hash_map();

//     /// Looks up the value associated with `key` and returns it, or nothing if the entry does not
//     /// exist.
//     option<V &> get(K const &key);
//     option<V const &> get(K const &key) const;

//     /// Removes an entry with key `key`. If the entry existed, it is returned.
//     option<V> remove(K const &key);

//     /// Inserts an entry into the map. If an entry with key `key` already existed, then that
//     entry
//     /// will be evicted and returned.
//     template <typename K2, typename V2>
//     option<V> insert(K2 &&key, V2 &&value);

//     bool key_exists(K const &key) const;

//     /// Returns the value associated with `key`, after inserting `value` into the map if the
//     entry
//     /// for `key` did not exist.
//     template <typename K2, typename V2>
//     V &get_or_insert(K2 &&key, V2 &&value);

//     /// Returns the value associated with `key`, after inserting the value returned by `producer`
//     /// into the map if the entry for `key` did not exist. The producer is only called when the
//     /// entry was vacant.
//     /// The signature for `producer` is `V()`
//     template <typename K2, typename F>
//     V &get_or_insert_with(K2 &&key, F producer);

//     /// Remove all entries from the map.
//     void clear();

//     // TODO: I don't like this method of iteration really...
//     /// Calls `func` on every occupied entry in the map until it returns false.
//     /// The signature for the callback is `bool(K&, V&)`.
//     template <typename F>
//     void iter(F func);

//     usize len() const;

//     /// The index operators will not do correctness checks!
//     V &operator[](K const &key);
//     V const &operator[](K const &key) const;

//     vector<collision<K>> find_collisions(allocator *alloc) const;
//     hashmap_stats stats() const;

// private:
//     void grow();
//     void try_grow();

//     option<usize> index_for(const K &key) const;
//     template <typename K2>
//     usize prepare_insertion(const K2 &key, u64 *hash_out);
//     usize next_index(usize i) const;
//     // Returns true if the cell at `i` is free to become used
//     bool is_vacant(usize i) const;
//     f64 load_factor() const;

//     allocator *alloc = nullptr;
//     usize capacity = 0, occupied = 0, items = 0;

//     u8 *control = nullptr;
//     K *keys = nullptr;
//     V *values = nullptr;
// };

// template <typename K, typename V, typename H>
// struct hash_map_keys_iterator : public std::iterator<std::forward_iterator_tag, > {
//     hash_map_keys_iterator() = default;
//     hash_map_keys_iterator(const hash_map_keys_iterator &other) = default;

//     hash_map_keys_iterator(hash_map<K, V, H> &iterating)
//         : iterating(std::addressof(iterating)), index(0) {
//         // We initialize with index 0, but the first slot might be unoccupied, so we have to
//         point
//         // the iterator to the correct spot.
//         ++*this;
//     }

//     bool operator==(const hash_map_keys_iterator &other) {
//         return index == other.index && iterating == other.iterating;
//     }

//     bool operator!=(const hash_map_keys_iterator &other) {
//         return index != other.index || iterating != other.iterating;
//     }

//     hash_map_keys_iterator &operator++() {
//         while (index < &&!iterating->is_vacant(++index))
//             ;
//     }

//     hash_map_keys_iterator operator++(int) {
//         hash_map_keys_iterator original = *this;
//         ++*this;
//         return original;
//     }

//     K &operator*() {
//         return iterating->keys[index];
//     }

//     K *operator->() {
//         return &iterating->keys[index];
//     }

//     usize index;
//     hash_map<K, V, H> *iterating;
// };

// } // namespace vixen

// #include "hashmap.inl"