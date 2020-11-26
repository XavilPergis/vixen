#pragma once

#include "vixen/slice.hpp"

namespace vixen {

#pragma region "slice Conversion"
// + ----- slice Conversion ----------------------------------------------------- +

template <typename T>
slice<T>::operator slice<const T>() const {
    return slice<const T>{ptr, len};
}

template <typename T>
const slice<T> &slice<T>::as_const() const {
    return *this;
}

#pragma endregion
#pragma region "slice Accessors"
// + ----- slice Accessors ------------------------------------------------------ +

template <typename T>
T &slice<T>::operator[](usize i) {
    _VIXEN_BOUNDS_CHECK(i, len);
    return ptr[i];
}

template <typename T>
const T &slice<T>::operator[](usize i) const {
    _VIXEN_BOUNDS_CHECK(i, len);
    return ptr[i];
}

template <typename T>
option<T &> slice<T>::first() {
    return len == 0 ? option<T &>() : ptr[0];
}

template <typename T>
option<T &> slice<T>::last() {
    return len == 0 ? option<T &>() : ptr[len - 1];
}

template <typename T>
option<const T &> slice<T>::first() const {
    return len == 0 ? option<const T &>() : ptr[0];
}

template <typename T>
option<const T &> slice<T>::last() const {
    return len == 0 ? option<const T &>() : ptr[len - 1];
}

template <typename T>
T *slice<T>::begin() {
    return ptr;
}

template <typename T>
T *slice<T>::end() {
    return &ptr[len];
}

template <typename T>
const T *slice<T>::begin() const {
    return ptr;
}

template <typename T>
const T *slice<T>::end() const {
    return &ptr[len];
}

#pragma endregion
#pragma region "slice Misc"
// + ----- slice Misc ----------------------------------------------------------- +

template <typename T>
inline bool operator==(slice<const T> const &lhs, slice<const T> const &rhs) {
    if (lhs.len != rhs.len) {
        return false;
    }
    for (usize i = 0; i < lhs.len; ++i) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

#pragma endregion
#pragma region "Traits"
// + ----- Traits --------------------------------------------------------------- +

template <typename T, typename H>
inline void hash(slice<T> const &values, H &hasher) {
    for (T const &value : values) {
        hash(value, hasher);
    }
}

#pragma endregion

} // namespace vixen
