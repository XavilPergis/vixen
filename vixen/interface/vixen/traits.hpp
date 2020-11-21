#pragma once

namespace vixen {
/// Like a destructor, but doesn't get called automatically at the end of a scope.
template <typename T>
void destroy(T &item) {}

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
    using item = T;
    using reference = T &;
    using const_reference = T const &;
    using pointer = T &;
    using const_pointer = T const &;
};

} // namespace vixen
