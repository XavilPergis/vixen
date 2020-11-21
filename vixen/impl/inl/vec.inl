#pragma once

#include "vixen/log.hpp"
#include "vixen/typeops.hpp"
#include "vixen/util.hpp"
#include "vixen/vec.hpp"

#include <algorithm>
#include <type_traits>

namespace vixen {

#pragma region "Initialization and Deinitialization"
// +------------------------------------------------------------------------------+
// | Initialization and Deinitialization                                          |
// +------------------------------------------------------------------------------+

template <typename T>
inline vector<T>::vector() {
    alloc = nullptr;
    data = nullptr;
    length = 0;
    capacity = 0;
}

template <typename T>
inline vector<T>::vector(allocator *alloc) {
    this->alloc = alloc;
    data = nullptr;
    length = 0;
    capacity = 0;
}

template <typename T>
inline vector<T>::vector(allocator *alloc, usize default_capacity) : vector(alloc) {
    set_capacity(default_capacity);
}

template <typename T>
inline vector<T>::vector(allocator *alloc, const vector<T> &other) : vector(alloc) {
    set_capacity(other.capacity);

    for (usize i = 0; i < other.len(); ++i) {
        push(copy_construct_maybe_allocator_aware(alloc, other[i]));
    }
}

template <typename T>
inline vector<T>::vector(vector<T> &&other)
    : alloc(util::exchange(other.alloc, nullptr))
    , data(util::exchange(other.data, nullptr))
    , length(util::exchange(other.length, 0))
    , capacity(util::exchange(other.capacity, 0)) {}

template <typename T>
inline vector<T> &vector<T>::operator=(vector<T> &&other) {
    if (std::addressof(other) == this)
        return *this;

    this->alloc = util::exchange(other.alloc, nullptr);
    this->data = util::exchange(other.data, nullptr);
    this->length = util::exchange(other.length, 0);
    this->capacity = util::exchange(other.capacity, 0);
    return *this;
}

template <typename T>
inline vector<T> vector<T>::clone(allocator *alloc) {
    return vector(alloc, *this);
}

template <typename T>
inline vector<T>::~vector() {
    if (capacity > 0) {
        alloc->dealloc(heap::layout::array_of<T>(capacity), data);
    }
}

#pragma endregion
#pragma region "Insertion and Removal"
// +------------------------------------------------------------------------------+
// | Insertion and Removal                                                        |
// +------------------------------------------------------------------------------+

template <typename T>
template <typename U>
inline void vector<T>::push(U &&value) {
    try_grow(1);
    util::construct_in_place(&data[length++], std::forward<U>(value));
}

// template <typename T>
// inline void Vec<T>::extend(slice<const T> elements) {
//     T *new_elems = reserve(elements.len);
//     util::copy(elements.ptr, new_elems, elements.len);
// }

template <typename T>
inline T *vector<T>::reserve(usize elements) {
    try_grow(elements);
    T *start = data + length;
    length += elements;
    return start;
}

template <typename T>
inline void vector<T>::truncate(usize len) {
    VIXEN_ENGINE_DEBUG_ASSERT(len <= length,
        "Tried to truncate vector to {} items, but the current length was {}.",
        len,
        length)
    for (usize i = len; i < length; ++i) {
        data[i].~T();
    }
    length = len;
}

template <typename T>
inline option<T> vector<T>::pop() {
    return length == 0 ? option<T>() : data[--length];
}

template <typename T>
inline T vector<T>::remove(usize idx) {
    VIXEN_ENGINE_DEBUG_ASSERT(length > idx,
        "Tried to remove element {} from a {}-element vector.",
        idx,
        length)

    swap(length - 1, idx);
    return data[--length];
}

template <typename T>
inline T vector<T>::shift_remove(usize idx) {
    VIXEN_ENGINE_DEBUG_ASSERT(length > idx,
        "Tried to remove element {} from a {}-element vector.",
        idx,
        length)

    T old_value = MOVE(data[idx]);
    util::copy(&data[idx + 1], &data[idx], length - idx + 1);
    --length;
    return old_value;
}

template <typename T>
inline void vector<T>::clear() {
    truncate(0);
}

#pragma endregion
#pragma region "Algorithms"
// +------------------------------------------------------------------------------+
// | Algorithms                                                                   |
// +------------------------------------------------------------------------------+

template <typename T>
inline void vector<T>::dedup_unstable() {
    for (usize i = 0; i < len(); ++i) {
        for (usize j = i + 1; j < len(); ++j) {
            if (data[i] == data[j]) {
                remove(j);
            }
        }
    }
}

template <typename T>
inline void vector<T>::dedup() {
    for (usize i = 0; i < len(); ++i) {
        for (usize j = i + 1; j < len(); ++j) {
            if (data[i] == data[j]) {
                shift_remove(j);
            }
        }
    }
}

template <typename T>
inline option<usize> vector<T>::index_of(const T &value) {
    for (usize i = 0; i < len(); ++i) {
        if (data[i] == value) {
            return i;
        }
    }
    return {};
}

template <typename T>
inline void vector<T>::swap(usize a, usize b) {
    VIXEN_ENGINE_DEBUG_ASSERT((length > a) && (length > b),
        "Tried swapping elements {} and {} in a(n) {}-element vector.",
        a,
        b,
        length)

    std::swap(data[a], data[b]);
}

#pragma endregion
#pragma region "Accessors"
// +------------------------------------------------------------------------------+
// | Accessors                                                                    |
// +------------------------------------------------------------------------------+

template <typename T>
inline option<T &> vector<T>::first() {
    return length == 0 ? option<T &>() : data[0];
}

template <typename T>
inline option<T &> vector<T>::last() {
    return length == 0 ? option<T &>() : data[length - 1];
}

template <typename T>
inline option<const T &> vector<T>::first() const {
    return length == 0 ? option<const T &>() : data[0];
}

template <typename T>
inline option<const T &> vector<T>::last() const {
    return length == 0 ? option<const T &>() : data[length - 1];
}

template <typename T>
inline T *vector<T>::begin() {
    return data;
}

template <typename T>
inline T *vector<T>::end() {
    return &data[length];
}

template <typename T>
inline const T *vector<T>::begin() const {
    return data;
}

template <typename T>
inline const T *vector<T>::end() const {
    return &data[length];
}

template <typename T>
inline const T &vector<T>::operator[](usize i) const {
    _VIXEN_BOUNDS_CHECK(i, length)
    return data[i];
}

template <typename T>
inline T &vector<T>::operator[](usize i) {
    _VIXEN_BOUNDS_CHECK(i, length)
    return data[i];
}

template <typename T>
inline usize vector<T>::len() const {
    return length;
}

#pragma endregion
#pragma region "Conversions"
// +------------------------------------------------------------------------------+
// | Conversions                                                                  |
// +------------------------------------------------------------------------------+
template <typename T>
inline const vector<T> &vector<T>::as_const() const {
    return *this;
}

template <typename T>
inline vector<T>::operator slice<const T>() const {
    return {data, length};
}

template <typename T>
inline vector<T>::operator slice<T>() {
    return {data, length};
}

#pragma endregion
#pragma region "Internal"
// +------------------------------------------------------------------------------+
// | Internal                                                                     |
// +------------------------------------------------------------------------------+

template <typename T>
inline usize vector<T>::next_capacity() {
    return capacity == 0 ? default_vec_capacity : 2 * capacity;
}

template <typename T>
inline void vector<T>::try_grow(usize elements_needed) {
    usize minimum_cap_needed = length + elements_needed;
    if (minimum_cap_needed >= capacity) {
        set_capacity(std::max(next_capacity(), minimum_cap_needed));
    }
}

template <typename T>
inline void vector<T>::set_capacity(usize cap) {
    VIXEN_ENGINE_ASSERT(alloc != nullptr, "Tried to grow a defaulted Vec.");

    data = (T *)alloc->realloc(heap::layout::array_of<T>(capacity),
        heap::layout::array_of<T>(cap),
        (void *)data);
    capacity = cap;
}

#pragma endregion
#pragma region "Traits"
// +------------------------------------------------------------------------------+
// | Traits                                                                       |
// +------------------------------------------------------------------------------+

template <typename T, typename H>
inline void hash(const vector<T> &values, H &hasher) {
    for (const T &value : values) {
        hash(value, hasher);
    }
}

#pragma endregion

} // namespace vixen
