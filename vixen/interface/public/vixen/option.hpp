#pragma once

#include "vixen/traits.hpp"
#include "vixen/types.hpp"
#include "vixen/util.hpp"

// clang-format disable
#include "option-storage.inl"
// clang-format enable

namespace vixen {

template <typename T>
struct option {
    using pointer = std::remove_reference_t<T> *;
    using const_pointer = const std::remove_reference_t<T> *;

    // Default constructor, initializes to unoccupied option.
    option();
    option(std::nullptr_t) : option() {}

    // Forwarding constructor/assignment operator from inner type
    template <typename U>
    option(U &&value);
    template <typename U>
    option<T> &operator=(U &&other);

    // Move constructor/assignment operator from
    option(option<T> &&other) = default;
    option<T> &operator=(option<T> &&other) = default;

    bool operator==(const option<T> &rhs) const;
    bool operator==(const T &rhs) const;
    bool operator!=(const option<T> &rhs) const;
    bool operator!=(const T &rhs) const;

    const T &operator*() const;
    T &operator*();
    const_pointer operator->() const;
    pointer operator->();

    explicit operator bool() const;
    bool is_some() const;
    bool is_none() const;

private:
    option_impl<T> impl;
};

template <typename T, typename H>
inline void hash(const option<T> &option, H &hasher);

} // namespace vixen

#include "option.inl"
