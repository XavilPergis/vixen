#pragma once

#include "vixen/assert.hpp"
#include "vixen/option.hpp"

namespace vixen {

#pragma region "option"
template <typename T>
inline option<T>::option() {}

template <typename T>
template <typename U>
inline option<T>::option(U &&value) {
    impl.set(std::forward<U>(value));
}

template <typename T>
template <typename U>
inline option<T> &option<T>::operator=(U &&other) {
    impl.set(std::forward<U>(other));
    return *this;
}

template <typename T>
inline bool option<T>::operator==(const option<T> &rhs) const {
    return is_none() ? rhs.is_none() : rhs.is_some() && impl.get() == rhs.internal_get();
}

template <typename T>
inline bool option<T>::operator==(const T &rhs) const {
    return is_some() && impl.get() == rhs;
}

template <typename T>
inline bool option<T>::operator!=(const option<T> &rhs) const {
    return !(*this == rhs);
}

template <typename T>
inline bool option<T>::operator!=(const T &rhs) const {
    return !(*this == rhs);
}

template <typename T>
inline option<T>::operator bool() const {
    return impl.get_occupied();
}

template <typename T>
inline const T &option<T>::operator*() const {
    VIXEN_DEBUG_ASSERT(impl.get_occupied(), "Tried to access empty option.");
    return impl.get();
}

template <typename T>
inline T &option<T>::operator*() {
    VIXEN_DEBUG_ASSERT(impl.get_occupied(), "Tried to access empty option.");
    return impl.get();
}

template <typename T>
inline typename option<T>::const_pointer option<T>::operator->() const {
    VIXEN_DEBUG_ASSERT(impl.get_occupied(), "Tried to access empty option.");
    return std::addressof(impl.get());
}

template <typename T>
inline typename option<T>::pointer option<T>::operator->() {
    VIXEN_DEBUG_ASSERT(impl.get_occupied(), "Tried to access empty option.");
    return std::addressof(impl.get());
}

template <typename T>
inline bool option<T>::is_some() const {
    return impl.get_occupied();
}

template <typename T>
inline bool option<T>::is_none() const {
    return !impl.get_occupied();
}

#pragma endregion
#pragma region "Traits"

template <typename T, typename H>
inline void hash(const option<T> &option, H &hasher) {
    // TODO: idk what we should hash in the None branch
    if (option) {
        hash(*option, hasher);
    } else {
        hash(0, hasher);
    }
}

#pragma endregion

} // namespace vixen
