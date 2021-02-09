#pragma once

#include <type_traits>

namespace vixen {

template <typename T, typename H>
inline void hash(const T &value, H &hasher) {
    hasher.write(value);
}

template <typename C>
struct is_collection : std::false_type {};

template <typename C>
struct collection_types {};

template <typename T>
struct standard_collection_types {
    using value_type = T;
    using reference = T &;
    using const_reference = T const &;
    using pointer = T &;
    using const_pointer = T const &;
};

struct copyable {};
struct noncopyable {
    noncopyable() = default;
    noncopyable(noncopyable const &) = delete;
    noncopyable &operator=(noncopyable const &) = delete;
    noncopyable(noncopyable &&) = default;
    noncopyable &operator=(noncopyable &&) = default;
};

struct moveable {};
struct nonmoveable {
    nonmoveable() = default;
    nonmoveable(nonmoveable const &) = default;
    nonmoveable &operator=(nonmoveable const &) = default;
    nonmoveable(nonmoveable &&) = delete;
    nonmoveable &operator=(nonmoveable &&) = delete;
};

struct defaultable {};
struct nondefaultable {
    nondefaultable() = delete;
    nondefaultable(nondefaultable const &) = default;
    nondefaultable &operator=(nondefaultable const &) = default;
    nondefaultable(nondefaultable &&) = default;
    nondefaultable &operator=(nondefaultable &&) = default;
};

// poisons generation of default copy constructors when `T` isn't copy-constructible.
template <typename T>
using mimic_copy_ctor = std::conditional_t<std::is_copy_constructible_v<T>, copyable, noncopyable>;

template <typename T>
using mimic_move_ctor = std::conditional_t<std::is_move_constructible_v<T>, moveable, nonmoveable>;

template <typename T>
using mimic_default_ctor
    = std::conditional_t<std::is_default_constructible_v<T>, defaultable, nondefaultable>;

template <typename T>
constexpr bool has_trivial_copy_ops
    = std::is_trivially_copy_constructible_v<T> &&std::is_trivially_copy_assignable_v<T>;

template <typename T>
constexpr bool has_trivial_move_ops
    = std::is_trivially_copy_constructible_v<T> &&std::is_trivially_copy_assignable_v<T>;

template <typename T>
constexpr bool has_trivial_destructor = std::is_trivially_destructible_v<T>;

} // namespace vixen
