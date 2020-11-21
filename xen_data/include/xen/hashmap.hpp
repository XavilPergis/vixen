#pragma once

#include "xen/allocator/allocator.hpp"
#include "xen/option.hpp"
#include "xen/vec.hpp"

#include <xen/types.hpp>

namespace xen {
using heap::layout;

// Number of slots in the initial allocation of a hashmap.
constexpr usize default_hashmap_capacity = 32;

// The ratio of occupied slots to free slots needed before a table resize
constexpr f64 hashmap_load_factor = 0.7;

constexpr u8 ctrl_deleted = 0b10000000;
constexpr u8 ctrl_free = 0b11111111;

// dumb dumb simple shitty hasher
struct fx_hasher {
    fx_hasher(u64 seed = 0);

    u64 finish();
    void add_to_hash(u64 i);
    void write_bytes(slice<const char> bytes);

    template <typename T>
    void write(const T &value);

    u64 current;
};

using default_hasher = fx_hasher;

template <typename T, typename H>
constexpr u64 make_hash(const T &value);

template <typename K>
struct collision {
    usize slot;
    K *current;
    u64 current_hash;
    bool collided_with_tombstone;
    option<K *> collided;
    option<u64> collided_hash;
    usize probe_chain_length;
};

struct hashmap_stats {
    usize length;
    usize occupied;
    usize items;
    usize collisions;
};

template <typename K, typename V, typename Hasher = default_hasher>
struct hash_map {
    hash_map();

    explicit hash_map(allocator *alloc);
    hash_map(allocator *alloc, usize default_capacity);
    template <typename OtherHasher>
    hash_map(allocator *alloc, const hash_map<K, V, OtherHasher> &other);
    hash_map(hash_map<K, V, Hasher> &&other);
    hash_map<K, V, Hasher> &operator=(hash_map<K, V, Hasher> &&other);

    hash_map<K, V, Hasher> clone(allocator *alloc) const;

    ~hash_map();

    /// Looks up the value associated with `key` and returns it, or nothing if the entry does not
    /// exist.
    option<V &> get(K const &key);
    option<V const &> get(K const &key) const;

    /// Removes an entry with key `key`. If the entry existed, it is returned.
    option<V> remove(K const &key);

    /// Inserts an entry into the map. If an entry with key `key` already existed, then that entry
    /// will be evicted and returned.
    template <typename K2, typename V2>
    option<V> insert(K2 &&key, V2 &&value);

    bool key_exists(K const &key) const;

    /// Returns the value associated with `key`, after inserting `value` into the map if the entry
    /// for `key` did not exist.
    template <typename K2, typename V2>
    V &get_or_insert(K2 &&key, V2 &&value);

    /// Returns the value associated with `key`, after inserting the value returned by `producer`
    /// into the map if the entry for `key` did not exist. The producer is only called when the
    /// entry was vacant.
    /// The signature for `producer` is `V()`
    template <typename K2, typename F>
    V &get_or_insert_with(K2 &&key, F producer);

    /// Remove all entries from the map. This does not destruct any entries, all it does it set
    /// in-use flags to none and length to 0.
    void clear();

    // TODO: I don't like this method of iteration really...
    /// Calls `func` on every occupied entry in the map until it returns false.
    /// The signature for the callback is `bool(K&, V&)`.
    template <typename F>
    void iter(F func);

    usize len() const;

    /// The index operators will not do correctness checks!
    V &operator[](K const &key);
    V const &operator[](K const &key) const;

    vector<collision<K>> find_collisions(allocator *alloc) const;
    hashmap_stats stats() const;

private:
    void grow();
    void try_grow();

    option<usize> index_for(const K &key) const;
    template <typename K2>
    usize prepare_insertion(const K2 &key, u64 *hash_out);
    usize next_index(usize i) const;
    // Returns true if the cell at `i` is free to become used
    bool is_vacant(usize i) const;
    f64 load_factor() const;

    allocator *alloc = nullptr;
    usize capacity = 0, occupied = 0, items = 0;

    u8 *control = nullptr;
    K *keys = nullptr;
    V *values = nullptr;
};

} // namespace xen

#include "inl/hashmap.inl"