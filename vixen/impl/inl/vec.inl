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
inline Vector<T>::Vector(copy_tag_t, Allocator *alloc, const Vector<T> &other) : Vector(alloc) {
    setCapacity(other.capacity);

    for (usize i = 0; i < other.length; ++i) {
        push(copy_construct_maybe_allocator_aware(alloc, other[i]));
    }
}

template <typename T>
inline Vector<T>::Vector(Vector<T> &&other)
    : alloc(util::exchange(other.alloc, nullptr))
    , data(util::exchange(other.data, nullptr))
    , length(util::exchange(other.length, 0))
    , capacity(util::exchange(other.capacity, 0)) {}

template <typename T>
inline Vector<T> &Vector<T>::operator=(Vector<T> &&other) {
    if (std::addressof(other) == this)
        return *this;

    this->alloc = util::exchange(other.alloc, nullptr);
    this->data = util::exchange(other.data, nullptr);
    this->length = util::exchange(other.length, 0);
    this->capacity = util::exchange(other.capacity, 0);
    return *this;
}

template <typename T>
inline Vector<T>::~Vector() {
    if (capacity > 0) {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (usize i = 0; i < length; ++i) {
                data[i].~T();
            }
        }
        alloc->dealloc(heap::Layout::array_of<T>(capacity), data);
    }
}

#pragma endregion
#pragma region "Insertion and Removal"
// +------------------------------------------------------------------------------+
// | Insertion and Removal                                                        |
// +------------------------------------------------------------------------------+

template <typename T, typename U, typename... Us>
constexpr void unpackToArray(T *location, U &&head, Us &&...tail) {
    util::construct_in_place(location, std::forward<U>(head));
    if constexpr (sizeof...(Us) > 0) {
        unpackToArray(location + 1, std::forward<Us>(tail)...);
    }
}

template <typename T>
template <typename... Us>
inline void Vector<T>::push(Us &&...values) {
    static_assert(sizeof...(Us) > 0, "cannot push zero elements to a vector");

    ensureCapacity(sizeof...(Us));
    unpackToArray<T, Us...>(&data[length], std::forward<Us>(values)...);
    length += sizeof...(Us);
}

template <typename T>
template <typename... Args>
inline void Vector<T>::emplace(Args &&...args) {
    ensureCapacity(1);
    util::construct_in_place(&data[length], std::forward<Args>(args)...);
    ++length;
}

template <typename T>
inline T *Vector<T>::reserve(usize elements) {
    ensureCapacity(elements);
    T *start = data + length;
    length += elements;
    return start;
}

template <typename T>
inline void Vector<T>::truncate(usize len) {
    VIXEN_DEBUG_ASSERT_EXT(len <= length,
        "tried to truncate vector to {} items, but the current length was {}",
        len,
        length);

    if (noexcept(std::declval<T>().~T())) {
        for (usize i = len; i < length; ++i) {
            data[i].~T();
        }
    } else {
        for (usize i = 0; i < length - len; ++i) {
            try {
                data[length - i - 1].~T();
            } catch (...) {
                length = i;
                throw;
            }
        }
    }

    length = len;
}

template <typename T>
inline Option<T> Vector<T>::pop() {
    return length == 0 ? Option<T>() : data[--length];
}

template <typename T>
inline T Vector<T>::remove(usize idx) {
    VIXEN_DEBUG_ASSERT_EXT(length > idx,
        "tried to remove element {} from a {}-element vector",
        idx,
        length);

    swap(length - 1, idx);
    return mv(data[--length]);
}

template <typename T>
inline T Vector<T>::shiftRemove(usize idx) {
    VIXEN_DEBUG_ASSERT_EXT(length > idx,
        "tried to remove element {} from a {}-element vector",
        idx,
        length);

    T old_value = mv(data[idx]);
    util::copy(&data[idx + 1], &data[idx], length - idx + 1);
    --length;
    return mv(old_value);
}

template <typename T>
inline void Vector<T>::clear() {
    truncate(0);
}

template <typename T>
template <typename... Us>
T *Vector<T>::insert(usize idx, Us &&...values) {
    VIXEN_DEBUG_ASSERT_EXT(idx <= length,
        "tried to insert element at {}, but the length was {}",
        idx,
        length);
    ensureCapacity(sizeof...(Us));

    if (idx == length) {
        // we don't need to guard exceptions here because we only grow the length *after* we unpack
        // all the new elements, so there's no possibility of accidentally including uninitialized
        // elements
        unpackToArray(&data[length], std::forward<Us>(values)...);
        length += sizeof...(Us);
    } else {
        // guard anything that we may have uninitialized at any point, so that if an exception is
        // thrown, no uninitialized data will be included in the vector. this satisfies the basic
        // exception safety guarntees.
        usize prevLength = length;
        length = idx;

        // this method of insertion does not preserve order, but makes insertion constant time. here
        // we carve out a chunk of space large enough to fit the new elements by moving all the
        // elements where the new ones would be to the end of the vector.
        usize shiftLength = util::min(sizeof...(Us), prevLength - idx);
        usize destStart = util::max(prevLength, idx + sizeof...(Us));
        for (usize i = 0; i < shiftLength; ++i) {
            util::construct_in_place(&data[destStart + i], mv(data[idx + i]));
            data[idx + i].~T();
        }
        unpackToArray(&data[idx], std::forward<Us>(values)...);

        length = prevLength + sizeof...(Us);
    }

    return &data[idx];
}

template <typename T>
template <typename... Us>
T *Vector<T>::shiftInsert(usize idx, Us &&...values) {
    VIXEN_DEBUG_ASSERT_EXT(idx <= length,
        "tried to insert element at {}, but the length was {}",
        idx,
        length);
    ensureCapacity(sizeof...(Us));

    if (idx == length) {
        // we don't need to guard exceptions here because we only grow the length *after* we unpack
        // all the new elements, so there's no possibility of accidentally including uninitialized
        // elements
        unpackToArray(&data[length], std::forward<Us>(values)...);
        length += sizeof...(Us);
    } else {
        // guard anything that we may have uninitialized at any point, so that if an exception is
        // thrown, no uninitialized data will be included in the vector. this satisfies the basic
        // exception safety guarntees.
        usize prevLength = length;
        length = idx;

        // shift everything down by the size of the parameter pack so we have enough room to
        // construct in-place at `idx`. since the source and destination of the move may overlap,
        // and we're expanding downwards, we have to copy from back to front.
        usize shiftLength = prevLength - idx;
        for (usize i = shiftLength; i-- > 0;) {
            util::construct_in_place(&data[idx + sizeof...(Us) + i], mv(data[idx + i]));
            data[idx + i].~T();
        }

        // in-place construction
        unpackToArray(&data[idx], std::forward<Us>(values)...);

        length = prevLength + sizeof...(Us);
    }

    return &data[idx];
}

#pragma endregion
#pragma region "Algorithms"
// +------------------------------------------------------------------------------+
// | Algorithms                                                                   |
// +------------------------------------------------------------------------------+

template <typename T>
inline void Vector<T>::dedupUnstable() {
    for (usize i = 0; i < length; ++i) {
        for (usize j = i + 1; j < length; ++j) {
            if (data[i] == data[j]) {
                remove(j);
            }
        }
    }
}

template <typename T>
inline void Vector<T>::dedup() {
    for (usize i = 0; i < length; ++i) {
        for (usize j = i + 1; j < length; ++j) {
            if (data[i] == data[j]) {
                shiftRemove(j);
            }
        }
    }
}

template <typename T>
inline Option<usize> Vector<T>::firstIndexOf(const T &value) {
    for (usize i = 0; i < length; ++i) {
        if (data[i] == value) {
            return i;
        }
    }

    return {};
}

template <typename T>
inline Option<usize> Vector<T>::lastIndexOf(const T &value) {
    for (usize i = 0; i < length; ++i) {
        if (data[length - i - 1] == value) {
            return length - i - 1;
        }
    }

    return {};
}

template <typename T>
inline void Vector<T>::swap(usize a, usize b) {
    VIXEN_DEBUG_ASSERT_EXT((length > a) && (length > b),
        "Tried swapping elements {} and {} in a(n) {}-element vector.",
        a,
        b,
        length);

    std::swap(data[a], data[b]);
}

#pragma endregion
#pragma region "Accessors"
// +------------------------------------------------------------------------------+
// | Accessors                                                                    |
// +------------------------------------------------------------------------------+

template <typename T>
inline Option<T &> Vector<T>::first() {
    return length == 0 ? Option<T &>() : data[0];
}

template <typename T>
inline Option<T &> Vector<T>::last() {
    return length == 0 ? Option<T &>() : data[length - 1];
}

template <typename T>
inline Option<const T &> Vector<T>::first() const {
    return length == 0 ? Option<const T &>() : data[0];
}

template <typename T>
inline Option<const T &> Vector<T>::last() const {
    return length == 0 ? Option<const T &>() : data[length - 1];
}

template <typename T>
inline T *Vector<T>::begin() {
    return data;
}

template <typename T>
inline T *Vector<T>::end() {
    return &data[length];
}

template <typename T>
inline const T *Vector<T>::begin() const {
    return data;
}

template <typename T>
inline const T *Vector<T>::end() const {
    return &data[length];
}

template <typename T>
inline const T &Vector<T>::operator[](usize i) const {
    _VIXEN_BOUNDS_CHECK(i, length);
    return data[i];
}

template <typename T>
inline T &Vector<T>::operator[](usize i) {
    _VIXEN_BOUNDS_CHECK(i, length);
    return data[i];
}

#pragma endregion
#pragma region "Conversions"
// +------------------------------------------------------------------------------+
// | Conversions                                                                  |
// +------------------------------------------------------------------------------+

template <typename T>
inline Vector<T>::operator Slice<const T>() const {
    return {data, length};
}

template <typename T>
inline Vector<T>::operator Slice<T>() {
    return {data, length};
}

template <typename T>
template <typename S>
S &Vector<T>::operator<<(S &s) {
    s << "[";
    if (length > 0) {
        s << data[0];
        for (usize i = 0; i < length; ++i) {
            s << ", " << data[i];
        }
    }
    s << "]";
    return s;
}

#pragma endregion
#pragma region "Internal"
// +------------------------------------------------------------------------------+
// | Internal                                                                     |
// +------------------------------------------------------------------------------+

template <typename T>
inline usize Vector<T>::nextCapacity() {
    return capacity == 0 ? default_vec_capacity : 2 * capacity;
}

template <typename T>
inline void Vector<T>::ensureCapacity(usize elements_needed) {
    usize minimum_cap_needed = length + elements_needed;
    if (minimum_cap_needed >= capacity) {
        setCapacity(std::max(nextCapacity(), minimum_cap_needed));
    }
}

template <typename T>
inline void Vector<T>::setCapacity(usize cap) {
    VIXEN_ASSERT_EXT(alloc != nullptr, "Tried to grow a vector with no allocator.");

    if (cap > 0) {
        data = (T *)alloc->realloc(heap::Layout::array_of<T>(capacity),
            heap::Layout::array_of<T>(cap),
            (void *)data);
        capacity = cap;
    }
}

#pragma endregion
#pragma region "Traits"
// +------------------------------------------------------------------------------+
// | Traits                                                                       |
// +------------------------------------------------------------------------------+

template <typename T, typename H>
inline void hash(const Vector<T> &values, H &hasher) {
    for (const T &value : values) {
        hash(value, hasher);
    }
}

#pragma endregion

} // namespace vixen
