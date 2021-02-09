#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/hash/hasher.hpp"

namespace vixen {

constexpr usize load_factor_numerator = 7;
constexpr usize load_factor_denomenator = 10;
constexpr usize default_hashmap_capacity = 16;

template <typename T>
struct collision {
    usize slot;
    T *current;
    u64 current_hash;
    bool collided_with_tombstone;
    option<T *> collided;
    option<u64> collided_hash;
    usize probe_chain_length;
};

struct hash_table_stats {
    usize length;
    usize occupied;
    usize items;
    usize collisions;
};

template <typename T>
struct default_comparator {
    template <typename U>
    constexpr static bool eq(const T &lhs, const U &rhs) {
        return lhs == rhs;
    }

    constexpr static const T &map_entry(const T &entry) {
        return entry.template get<0>();
    }
};

/// @ingroup vixen_data_structures
template <typename T, typename Hasher = default_hasher, typename Cmp = default_comparator<T>>
struct hash_table {
    constexpr hash_table() = default;

    constexpr explicit hash_table(allocator *alloc) : alloc(alloc) {}
    hash_table(allocator *alloc, usize default_capacity);
    hash_table(allocator *alloc, const hash_table &other);
    constexpr hash_table(hash_table &&other);
    constexpr hash_table &operator=(hash_table &&other);

    ~hash_table();

    void insert(usize slot, u64 hash, T &&value);
    constexpr void remove(usize slot);
    constexpr T &get(usize slot);
    constexpr const T &get(usize slot) const;
    constexpr bool is_occupied(usize slot) const;

    constexpr void clear();
    void grow();

    template <typename OT>
    constexpr usize find_insert_slot(u64 hash, const OT &value) const;
    template <typename OT>
    constexpr option<usize> find_slot(u64 hash, const OT &value) const;

    constexpr bool does_table_need_resize() const;
    constexpr void insert_no_resize(usize slot, u64 hash, T &&value);

    /// @brief Returns the number of entries in the table.
    // clang-format off
    constexpr usize len() const { return items; }
    // clang-format on

    allocator *alloc;

    usize capacity = 0;
    usize occupied = 0;
    usize items = 0;

    u8 *control = nullptr;
    T *buckets = nullptr;
};

// template <typename T, typename H, typename C>
// struct hash_table_iterator {
//     hash_table_iterator(const hash_table_iterator &other) = default;
//     hash_table_iterator(hash_table<T, H, C> &iterating) : iterating(iterating) {}

//     struct reified {
//         hash_table_iterator(hash_table_iterator &&desc) : iterating(desc.iterating) {}

//         using output = T &;

//         bool has_next() {}

//         output next() {
//             output &ret = iterating.get(current_slot++);
//             while (iterating.is_occupied())
//         }

//         usize current_slot = 0;
//         hash_table<T, H, C> &iterating;
//     };

//     hash_table<T, H, C> &iterating;
// };

// template <typename T, typename H>
// struct hash_table_iterator : public std::iterator<std::forward_iterator_tag, T> {
//     hash_table_iterator() = default;
//     hash_table_iterator(const hash_table_iterator &other) = default;

//     hash_table_iterator(hash_table<T, H> &iterating)
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
//     hash_table<T, H> *iterating;
// };

} // namespace vixen

#include "vixen/bits/hash/table.inl"
