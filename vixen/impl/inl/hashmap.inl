// #pragma once

// #include "vixen/allocator/allocator.hpp"
// #include "vixen/hashmap.hpp"
// #include "vixen/option.hpp"
// #include "vixen/traits.hpp"
// #include "vixen/types.hpp"
// #include "vixen/util.hpp"

// namespace vixen {

// #pragma region "Hashmap Initialization and Deinitialization"
// // + ----- Hashmap Initialization and Deinitialization -------------------------- +

// template <typename K, typename V, typename H>
// inline hash_map<K, V, H>::hash_map() {}

// template <typename K, typename V, typename H>
// inline hash_map<K, V, H>::hash_map(allocator *alloc) : alloc(alloc) {}

// template <typename K, typename V, typename H>
// inline hash_map<K, V, H>::hash_map(allocator *alloc, usize default_capacity) : hash_map(alloc) {
//     if (default_capacity > 0) {
//         control = heap::create_array_init<u8>(alloc, default_capacity, ctrl_free);
//         keys = heap::create_array_uninit<K>(alloc, default_capacity);
//         values = heap::create_array_uninit<V>(alloc, default_capacity);
//         capacity = default_capacity;
//     }
// }

// template <typename K, typename V, typename H>
// template <typename OtherHasher>
// inline hash_map<K, V, H>::hash_map(allocator *alloc, const hash_map<K, V, OtherHasher> &other) {}

// template <typename K, typename V, typename H>
// inline hash_map<K, V, H>::hash_map(hash_map<K, V, H> &&other)
//     : alloc(util::exchange(other.alloc, nullptr))
//     , capacity(util::exchange(other.capacity, 0))
//     , occupied(util::exchange(other.occupied, 0))
//     , items(util::exchange(other.items, 0))
//     , control(util::exchange(other.control, nullptr))
//     , keys(util::exchange(other.keys, nullptr))
//     , values(util::exchange(other.values, nullptr)) {}

// template <typename K, typename V, typename H>
// inline hash_map<K, V, H> &hash_map<K, V, H>::operator=(hash_map<K, V, H> &&other) {
//     if (std::addressof(other) == this)
//         return *this;

//     this->alloc = util::exchange(other.alloc, nullptr);
//     this->capacity = util::exchange(other.capacity, 0);
//     this->occupied = util::exchange(other.occupied, 0);
//     this->items = util::exchange(other.items, 0);
//     this->control = util::exchange(other.control, nullptr);
//     this->keys = util::exchange(other.keys, nullptr);
//     this->values = util::exchange(other.values, nullptr);
//     return *this;
// }

// // template <typename K, typename V, typename H>
// // inline hash_map<K, V, H>::hash_map() {}

// template <typename K, typename V, typename H>
// inline hash_map<K, V, H>::~hash_map() {
//     if (capacity == 0)
//         return;

//     for (usize i = 0; i < capacity; ++i) {
//         if (!is_vacant(i)) {
//             keys[i].~K();
//             values[i].~V();
//         }
//     }

//     heap::destroy_array<u8>(alloc, control, capacity);
//     heap::destroy_array<K>(alloc, keys, capacity);
//     heap::destroy_array<V>(alloc, values, capacity);
// }

// #pragma endregion
// #pragma region "hash_map Operations"
// // + ----- hash_map Operations --------------------------------------------------- +

// template <typename K, typename V, typename Hasher>
// inline option<V &> hash_map<K, V, Hasher>::get(const K &key) {
//     if (capacity == 0) {
//         return {};
//     }

//     if (auto idx = index_for(key)) {
//         return values[*idx];
//     }
//     return {};
// }

// template <typename K, typename V, typename Hasher>
// inline option<V> hash_map<K, V, Hasher>::remove(const K &key) {
//     if (capacity == 0) {
//         return {};
//     }

//     if (auto i = index_for(key)) {
//         // Let's say we have three keys that we want to insert: A, B, and C. First A is
//         // inserted into slot 0, then B into slot 1. When C is inserted, its hash is the
//         // same as A's, but the first available slot is at index 2, so C is inserted there.
//         // Then, B is removed. If B's slot were to be marked as free, later searches for C
//         // would come up empty, as the search would not get past the now free slot.
//         //
//         // To remedy this, we insert a "tombstone" in B's slot if it's not the last in a
//         // search chain. This tombstone can be populated again with a live item just like a
//         // free slot, but critically *does not stop a search*.
//         bool can_free = control[next_index(*i)] == ctrl_free;
//         control[*i] = can_free ? ctrl_free : ctrl_deleted;
//         occupied -= can_free;
//         items -= 1;
//         return mv(values[*i]);
//     }

//     return {};
// }

// template <typename K, typename V, typename Hasher>
// template <typename K2, typename V2>
// inline option<V> hash_map<K, V, Hasher>::insert(K2 &&key, V2 &&value) {
//     try_grow();

//     u64 hash;
//     usize index = prepare_insertion(key, &hash);

//     option<V> old_value;
//     if (!is_vacant(index)) {
//         old_value = mv(values[index]);
//     }

//     util::construct_in_place(&keys[index], std::forward<K2>(key));
//     util::construct_in_place(&values[index], std::forward<V2>(value));

//     occupied += control[index] == ctrl_free;
//     items += 1;
//     control[index] = (detail::extract_h2(hash) & 0x7f) % capacity;

//     return old_value;
// }

// template <typename K, typename V, typename Hasher>
// inline bool hash_map<K, V, Hasher>::key_exists(const K &key) const {
//     if (capacity == 0) {
//         return false;
//     }

//     return (bool)index_for(key);
// }

// template <typename K, typename V, typename H>
// template <typename K2, typename V2>
// inline V &hash_map<K, V, H>::get_or_insert(K2 &&key, V2 &&value) {
//     if (auto index = index_for(key)) {
//         return values[*index];
//     }

//     try_grow();

//     u64 hash;
//     usize index = prepare_insertion(key, &hash);
//     util::construct_in_place(&keys[index], std::forward<K2>(key));
//     util::construct_in_place(&values[index], std::forward<V2>(value));

//     occupied += control[index] == ctrl_free;
//     items += 1;
//     control[index] = (detail::extract_h2(hash) & 0x7f) % capacity;

//     return values[index];
// }

// template <typename K, typename V, typename H>
// template <typename K2, typename F>
// inline V &hash_map<K, V, H>::get_or_insert_with(K2 &&key, F producer) {
//     if (auto index = index_for(key)) {
//         return values[*index];
//     }

//     try_grow();

//     u64 hash;
//     usize index = prepare_insertion(key, &hash);
//     util::construct_in_place(&keys[index], std::forward<K2>(key));
//     util::construct_in_place(&values[index], producer());

//     occupied += control[index] == ctrl_free;
//     items += 1;
//     control[index] = (detail::extract_h2(hash) & 0x7f) % capacity;

//     return values[index];
// }

// template <typename K, typename V, typename H>
// inline void hash_map<K, V, H>::clear() {
//     for (usize i = 0; i < capacity; i++) {
//         if (!is_vacant(i)) {
//             keys[i].~K();
//             values[i].~V();
//             control[i] = ctrl_free;
//         }
//     }

//     occupied = 0;
//     items = 0;
// }

// template <typename K, typename V, typename H>
// template <typename F>
// inline void hash_map<K, V, H>::iter(F func) {
//     for (usize i = 0; i < capacity; i++) {
//         if (!is_vacant(i)) {
//             func(keys[i], values[i]);
//         }
//     }
// }

// #pragma endregion
// #pragma region "hash_map Accessors"
// // + ----- hash_map Accessors ---------------------------------------------------- +

// template <typename K, typename V, typename H>
// usize hash_map<K, V, H>::len() const {
//     return items;
// }

// template <typename K, typename V, typename H>
// V &hash_map<K, V, H>::operator[](K const &key) {
//     auto idx = index_for(key);
//     VIXEN_DEBUG_ASSERT(idx.is_some(), "tried to access value for non-existent key {}", key);
//     return values[*idx];
// }

// template <typename K, typename V, typename H>
// V const &hash_map<K, V, H>::operator[](K const &key) const {
//     auto idx = index_for(key);
//     VIXEN_DEBUG_ASSERT(idx.is_some(), "tried to access value for non-existent key {}", key);
//     return values[*idx];
// }

// #pragma endregion
// #pragma region "hash_map stats"
// // + ----- hash_map stats -------------------------------------------------------- +

// template <typename K, typename V, typename H>
// hashmap_stats hash_map<K, V, H>::stats() const {
//     hashmap_stats stats;

//     stats.length = capacity;
//     stats.occupied = occupied;
//     stats.items = items;
//     stats.collisions = 0;

//     for (usize i = 0; i < capacity; ++i) {
//         if (!is_vacant(i)) {
//             u64 hash = make_hash<K, H>(keys[i]);
//             u64 actual = detail::extract_h1(hash) % capacity;

//             // See `find_collisions`
//             stats.collisions += actual != i;
//         }
//     }

//     return stats;
// };

// template <typename K, typename V, typename H>
// vector<collision<K>> hash_map<K, V, H>::find_collisions(allocator *alloc) const {
//     vector<collision<K>> collisions(alloc);

//     for (usize i = 0; i < capacity; ++i) {
//         if (!is_vacant(i)) {
//             u64 hash = make_hash<K, H>(keys[i]);
//             u64 actual = detail::extract_h1(hash) % capacity;

//             // If the hash of the current key points to the same slot, then it is "not
//             // colliding" with anything. Note that collision is not a symmetric relationship, so
//             // a slot can have no collisions itself, while there are collisions *to* that slot.
//             if (actual == i) {
//                 continue;
//             }

//             collision<K> *coll = collisions.reserve(1);
//             coll->slot = actual;
//             coll->current = &keys[i];
//             coll->current_hash = hash;
//             coll->collided_with_tombstone = control[actual] == ctrl_deleted;
//             coll->collided = control[actual] == ctrl_deleted ? option<K *>() : &keys[actual];
//             coll->collided_hash
//                 = control[actual] == ctrl_deleted ? option<u64>() : make_hash<K,
//                 H>(keys[actual]);

//             // Assumes linear probing. If the hash wraps around, then it's the sum of the
//             distance
//             // between the actual slot and the end of the array, the distance between the start
//             of
//             // the array and the actual insert spot (which is just the insert spot due to a
//             // subtraction by 0), and the "distance" between the end and start of the array,
//             which
//             // is always 1.
//             coll->probe_chain_length = i > actual ? i - actual : 1 + (capacity - actual) + i;
//         }
//     }

//     return collisions;
// }

// #pragma endregion
// #pragma region "hash_map Internal"
// // + ----- hash_map Internal ----------------------------------------------------- +

// template <typename K, typename V, typename H>
// inline void hash_map<K, V, H>::grow() {
//     usize new_len = capacity == 0 ? default_hashmap_capacity : capacity * 2;

//     hash_map<K, V, H> new_map(alloc, new_len);
//     for (usize i = 0; i < capacity; i++) {
//         if (!is_vacant(i)) {
//             new_map.insert(mv(keys[i]), mv(values[i]));
//         }
//     }

//     *this = mv(new_map);
// }

// template <typename K, typename V, typename H>
// inline void hash_map<K, V, H>::try_grow() {
//     if (capacity == 0 || load_factor() >= hashmap_load_factor) {
//         grow();
//     }
// }

// template <typename K, typename V, typename Hasher>
// inline option<usize> hash_map<K, V, Hasher>::index_for(const K &key) const {
//     u64 hash = make_hash<K, Hasher>(key);
//     u64 hash1 = detail::extract_h1(hash) % capacity;
//     u8 hash2 = detail::extract_h2(hash) % capacity;

//     for (usize i = hash1;; i = next_index(i)) {
//         // Note that the hash2 comparison also acts as an existence check, as a vacant slot has
//         // the highest bit set, and the mask with 0x7f removes the highest bit from the computed
//         // hash.
//         if (((hash2 & 0x7f) == control[i]) && likely(key == keys[i])) {
//             return i;
//         }

//         // We hit the end of this probe chain and didn't find a matching key
//         if (control[i] == ctrl_free) {
//             return option<usize>();
//         }
//     }
// }

// template <typename K, typename V, typename Hasher>
// template <typename K2>
// inline usize hash_map<K, V, Hasher>::prepare_insertion(const K2 &key, u64 *hash_out) {
//     u64 hash = make_hash<K, Hasher>(key);
//     u64 hash1 = detail::extract_h1(hash) % capacity;
//     u8 hash2 = detail::extract_h2(hash) % capacity;

//     // We didn't find the key in the map already, so it's good to go ahead and search for an
//     empty
//     // slot to insert into.
//     for (usize i = hash1;; i = next_index(i)) {
//         if (!is_vacant(i)) {
//             if ((hash2 & 0x7f) != control[i])
//                 continue;

//             if (unlikely(key != keys[i]))
//                 continue;
//         }

//         if (hash_out)
//             *hash_out = hash;

//         return i;
//     }

//     VIXEN_UNREACHABLE();
// }

// template <typename K, typename V, typename H>
// usize hash_map<K, V, H>::next_index(usize i) const {
//     _VIXEN_BOUNDS_CHECK(i, capacity);
//     return (i + 1) % capacity;
// }

// // Returns true if the cell at `i` is free to become used
// template <typename K, typename V, typename H>
// bool hash_map<K, V, H>::is_vacant(usize i) const {
//     _VIXEN_BOUNDS_CHECK(i, capacity);
//     // An active top bit signals vacancy.
//     return (control[i] & 0x80) != 0;
// }

// template <typename K, typename V, typename H>
// f64 hash_map<K, V, H>::load_factor() const {
//     return (f64)occupied / (f64)capacity;
// }

// } // namespace vixen
