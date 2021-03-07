#pragma once

#include "vixen/hash/map.hpp"

namespace vixen {

template <typename K, typename V, typename H, typename C>
constexpr HashMap<K, V, H, C>::HashMap(Allocator *alloc) : table(alloc) {}

template <typename K, typename V, typename H, typename C>
inline HashMap<K, V, H, C>::HashMap(Allocator *alloc, usize default_capacity)
    : table(alloc, default_capacity) {}

template <typename K, typename V, typename H, typename C>
inline HashMap<K, V, H, C>::HashMap(copy_tag_t, Allocator *alloc, const HashMap &other)
    : table(other.table.clone(alloc)) {}

template <typename K, typename V, typename H, typename C>
constexpr Option<V &> HashMap<K, V, H, C>::get(const K &key) {
    if (auto slot_opt = table.find_slot(make_hash<H>(key), key)) {
        return table.get(*slot_opt).template get<1>();
    }
    return empty_opt;
}

template <typename K, typename V, typename H, typename C>
constexpr Option<V const &> HashMap<K, V, H, C>::get(const K &key) const {
    if (auto slot_opt = table.find_slot(make_hash<H>(key), key)) {
        return table.get(*slot_opt).template get<1>();
    }
    return empty_opt;
}

template <typename K, typename V, typename H, typename C>
constexpr Option<V> HashMap<K, V, H, C>::remove(const K &key) {
    if (auto slot = table.find_slot(make_hash<H>(key), key)) {
        auto old_entry = mv(table.get(*slot));
        table.remove(*slot);
        return mv(old_entry.template get<1>());
    }
    return empty_opt;
}

template <typename K, typename V, typename H, typename C>
template <typename OK, typename OV>
inline Option<V> HashMap<K, V, H, C>::insert(OK &&key, OV &&value) {
    table.grow();

    auto hash = make_hash<H>(key);
    auto slot = table.find_insert_slot(hash, key);

    Option<V> old;
    if (table.is_occupied(slot)) {
        auto old_entry = mv(table.get(slot));
        old = mv(old_entry.template get<1>());
    }

    Tuple<K, V> entry{std::forward<OK>(key), std::forward<OV>(value)};
    table.insert(slot, hash, mv(entry));
    return old;
}

template <typename K, typename V, typename H, typename C>
template <typename OK, typename... Args>
inline V &HashMap<K, V, H, C>::get_or_insert(OK &&key, Args &&...value) {
    auto hash = make_hash<H>(key);
    if (auto existing_slot = table.find_slot(hash, key)) {
        return table.get(existing_slot.get()).template get<1>();
    }

    table.grow();

    auto slot = table.find_insert_slot(hash, key);
    Tuple<K, V> entry{std::forward<OK>(key), V{std::forward<Args>(value)...}};
    table.insert(slot, hash, mv(entry));
    return table.get(slot).template get<1>();
}

template <typename K, typename V, typename H, typename C>
constexpr Option<HashMapEntry<K, V, H, C>> HashMap<K, V, H, C>::entry(K const &key) {
    if (auto slot_opt = table.find_slot(make_hash<H>(key), key)) {
        return HashMapEntry<K, V, H, C>(this, *slot_opt);
    }
    return empty_opt;
}

template <typename K, typename V, typename H, typename C>
constexpr bool HashMap<K, V, H, C>::contains_key(const K &key) const {
    return (bool)table.find_slot(make_hash<H>(key), key);
}

template <typename K, typename V, typename H, typename C>
constexpr void HashMap<K, V, H, C>::clear() {
    table.clear();
}

// @TODO/Debugging: When accessing a nonexistent key, print it in the panic info, and fallback if
// `<<` to a stream on `K` doesn't exist.
template <typename K, typename V, typename H, typename C>
constexpr V &HashMap<K, V, H, C>::operator[](K const &key) {
    auto slot = table.find_slot(make_hash<H>(key), key);
    VIXEN_ASSERT_EXT(slot.is_some(), "Tried to access item that does not exist.");
    return table.get(*slot).template get<1>();
}

template <typename K, typename V, typename H, typename C>
constexpr V const &HashMap<K, V, H, C>::operator[](K const &key) const {
    auto slot = table.find_slot(make_hash<H>(key), key);
    VIXEN_ASSERT_EXT(slot.is_some(), "Tried to access item that does not exist.");
    return table.get(*slot).template get<1>();
}

template <typename K, typename V, typename H, typename C>
struct HashMapEntryForwardIterator {
    using iterator_category = std::forward_iterator_tag;
    using value_type = Tuple<K, V>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;

    HashMap<K, V, H, C> *iterating = nullptr;
    usize slot = 0;
    bool is_at_end = false;

    HashMapEntryForwardIterator(HashMap<K, V, H, C> *iterating) : iterating(iterating) {
        move_next();
    }

    HashMapEntryForwardIterator() = default;
    HashMapEntryForwardIterator(const HashMapEntryForwardIterator &) = default;
    HashMapEntryForwardIterator &operator=(const HashMapEntryForwardIterator &) = default;

    bool operator==(const HashMapEntryForwardIterator &other) const {
        if (is_at_end) {
            return other.is_at_end && iterating == other.iterating;
        } else {
            return !other.is_at_end && iterating == other.iterating && slot == other.slot;
        }
    }
    bool operator!=(const HashMapEntryForwardIterator &other) const {
        if (is_at_end) {
            return !other.is_at_end || iterating != other.iterating;
        } else {
            return other.is_at_end || iterating != other.iterating || slot != other.slot;
        }
    }

    HashMapEntryForwardIterator &operator++() {
        slot += 1;
        move_next();
        return *this;
    }

    HashMapEntryForwardIterator operator++(int) {
        HashMapEntryForwardIterator ret(*this);
        slot += 1;
        move_next();
        return ret;
    }

    reference operator*() const {
        return iterating->table.get(slot);
    }

    pointer operator->() const {
        return &iterating->table.get(slot);
    }

    void move_next() {
        while (true) {
            if (slot >= iterating->table.capacity) {
                is_at_end = true;
                return;
            } else if (!iterating->table.is_occupied(slot)) {
                slot += 1;
            } else {
                return;
            }
        }
    }
};

template <typename K, typename V, typename H, typename C>
constexpr HashMapEntryForwardIterator<K, V, H, C> HashMap<K, V, H, C>::begin() {
    return HashMapEntryForwardIterator<K, V, H, C>(this);
}

template <typename K, typename V, typename H, typename C>
constexpr HashMapEntryForwardIterator<K, V, H, C> HashMap<K, V, H, C>::end() {
    HashMapEntryForwardIterator<K, V, H, C> iter(this);
    iter.is_at_end = true;
    return iter;
}

template <typename K, typename V, typename H, typename C>
constexpr K const &HashMapEntry<K, V, H, C>::key() const {
    VIXEN_DEBUG_ASSERT_EXT(slot.is_some(), "tried to access key of empty map entry.");
    return map_in->table.get(*slot).template get<0>();
}

template <typename K, typename V, typename H, typename C>
constexpr V const &HashMapEntry<K, V, H, C>::value() const {
    VIXEN_DEBUG_ASSERT_EXT(slot.is_some(), "tried to access const value of empty map entry.");
    return map_in->table.get(*slot).template get<1>();
}

template <typename K, typename V, typename H, typename C>
constexpr V &HashMapEntry<K, V, H, C>::value() {
    VIXEN_DEBUG_ASSERT_EXT(slot.is_some(), "tried to access value of empty map entry.");
    return map_in->table.get(*slot).template get<1>();
}

template <typename K, typename V, typename H, typename C>
constexpr Tuple<K, V> HashMapEntry<K, V, H, C>::remove() {
    VIXEN_DEBUG_ASSERT_EXT(slot.is_some(), "tried to remove an already empty map entry.");
    auto ret = mv(map_in->table.get(*slot));
    map_in->table.remove(*slot);
    slot.clear();
    return ret;
}

} // namespace vixen
