#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/hash/hasher.hpp"

namespace vixen {

// keep table under 70% full.
constexpr usize LOAD_FACTOR_NUMERATOR = 7;
constexpr usize LOAD_FACTOR_DENOMENATOR = 10;
constexpr usize DEFAULT_HASHMAP_CAPACITY = 16;

template <typename T>
struct HashTableCollision {
    /**
     * @brief the hash table slot this entry occupies.
     */
    usize slot;

    /**
     * @brief a pointer to the entry that was queried.
     */
    T *queried;

    /**
     * @brief the hash of the queried entry.
     */
    u64 queriedHash;

    /**
     * @brief true if there was a collision, but it was with a tombstone sentinil.
     */
    bool collidedWithTombstone;

    /**
     * @brief a pointer to the entry that was collided with. Empty if there was no collision.
     */
    Option<T *> collided;

    /**
     * @brief the hash of the collided entry.
     *
     */
    Option<u64> collidedHash;

    /**
     * @brief The amount of "hops" that would need to be preformed before an open slot is found.
     */
    usize probeChainLength;
};

struct HashTableStats {
    usize length;
    usize occupied;
    usize items;
    usize collisions;
};

template <typename T>
struct default_comparator {
    template <typename U>
    constexpr static bool eq(const T &lhs, const U &rhs) noexcept {
        return lhs == rhs;
    }

    constexpr static const T &map_entry(const T &entry) noexcept { return entry.template get<0>(); }
};

/// @ingroup vixen_data_structures
template <typename T, typename Hasher = default_hasher, typename Cmp = default_comparator<T>>
struct HashTable {
    constexpr HashTable() = default;

    constexpr explicit HashTable(Allocator &alloc) noexcept : alloc(&alloc) {}
    HashTable(Allocator &alloc, usize default_capacity);
    HashTable(copy_tag_t, Allocator &alloc, const HashTable &other);
    constexpr HashTable(HashTable &&other) noexcept;
    constexpr HashTable &operator=(HashTable &&other) noexcept;

    VIXEN_DEFINE_CLONE_METHOD(HashTable)

    ~HashTable();

    void insert(usize slot, u64 hash, T &&value);
    constexpr void remove(usize slot) noexcept;
    constexpr T &get(usize slot) noexcept;
    constexpr const T &get(usize slot) const noexcept;
    constexpr bool isOccupied(usize slot) const noexcept;

    constexpr void removeAll() noexcept;
    void grow();

    template <typename OT>
    constexpr usize findInsertSlot(u64 hash, const OT &value) const noexcept;
    template <typename OT>
    constexpr Option<usize> findSlot(u64 hash, const OT &value) const noexcept;

    constexpr bool doesTableNeedResize() const noexcept;
    constexpr void insertNoResize(usize slot, u64 hash, T &&value) noexcept;

    /// @brief Returns the number of entries in the table.
    // clang-format off
    constexpr usize len() const { return items; }
    // clang-format on

    Allocator *alloc = nullptr;

    usize capacity = 0;
    usize occupied = 0;
    usize items = 0;

    u8 *control = nullptr;
    T *buckets = nullptr;
};

// template <typename T, typename H, typename C>
// struct hash_table_iterator {
//     hash_table_iterator(const hash_table_iterator &other) = default;
//     hash_table_iterator(HashTable<T, H, C> &iterating) : iterating(iterating) {}

//     struct reified {
//         hash_table_iterator(hash_table_iterator &&desc) : iterating(desc.iterating) {}

//         using output = T &;

//         bool has_next() {}

//         output next() {
//             output &ret = iterating.get(current_slot++);
//             while (iterating.isOccupied())
//         }

//         usize current_slot = 0;
//         HashTable<T, H, C> &iterating;
//     };

//     HashTable<T, H, C> &iterating;
// };

// template <typename T, typename H>
// struct hash_table_iterator : public std::iterator<std::forward_iterator_tag, T> {
//     hash_table_iterator() = default;
//     hash_table_iterator(const hash_table_iterator &other) = default;

//     hash_table_iterator(HashTable<T, H> &iterating)
//         : iterating(std::addressof(iterating)), index(0) {
//         // We initialize with index 0, but the first slot might be unoccupied, so we have to
//         point
//         // the iterator to the correct spot.
//         ++*this;
//     }

//     bool operator==(const hash_table_iterator &other) {
//         return index == other.index && iterating == other.iterating;
//     }

//     bool operator!=(const hash_table_iterator &other) {
//         return index != other.index || iterating != other.iterating;
//     }

//     hash_table_iterator &operator++() {
//         while (index < iterating->capacity && !iterating->is_vacant(++index))
//             ;
//     }

//     hash_table_iterator operator++(int) {
//         hash_table_iterator original = *this;
//         ++*this;
//         return original;
//     }

//     T &operator*() {
//         return iterating->keys[index];
//     }

//     T *operator->() {
//         return &iterating->keys[index];
//     }

//     usize index;
//     HashTable<T, H> *iterating;
// };

} // namespace vixen

#include "vixen/bits/hash/table.inl"