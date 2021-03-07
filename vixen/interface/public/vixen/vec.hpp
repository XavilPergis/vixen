#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/option.hpp"
#include "vixen/slice.hpp"
#include "vixen/traits.hpp"
#include "vixen/types.hpp"

/// @defgroup vixen_data_structures Data Structures

namespace vixen {
// Number of elements in the initial allocation of a vector.
constexpr const usize default_vec_capacity = 8;

/// @ingroup vixen_data_structures
/// @brief Growable dynamically-allocated array.
///
/// @warning Any reference into a vector is only valid until the vector's size changes.
template <typename T>
struct Vector {
    /// Default construction of a vector results in a vector in the moved-from state.
    Vector() = default;
    Vector(Vector const &) = delete;
    Vector &operator=(Vector const &) = delete;
    Vector(Vector<T> &&other);
    Vector<T> &operator=(Vector<T> &&other);
    ~Vector();

    Vector(copy_tag_t, Allocator *alloc, const Vector<T> &other);

    explicit Vector(Allocator *alloc) : alloc(alloc) {}
    Vector(Allocator *alloc, usize default_capacity) : alloc(alloc) {
        set_capacity(default_capacity);
    }

    VIXEN_DEFINE_CLONE_METHOD(Vector);

    /// Appends a single item to the end of the vector.
    template <typename... Args>
    void push(Args &&...values);

    /// Extends the vector by `elements` elements, and returns a pointer to the start of the
    /// newly allocated elements.
    ///
    /// @warning This method does _not_ initialize any elements.
    T *reserve(usize elements = 1);

    /// Sets the length of the vector to `len` items long.
    void truncate(usize len);

    /// Removes the last item from the vector and returns it, or en empty option if there is nothing
    /// to pop.
    Option<T> pop();

    /// Removes an item from the list efficiently by swapping it with the last element and
    /// popping. This operation does not preserve ordering. O(1)
    T remove(usize idx);

    /// Removes an item for the list by shifting all elements past `idx` back one. O(n)
    T shift_remove(usize idx);

    template <typename U>
    T &shift_insert(usize idx, U &&val);

    Option<usize> index_of(const T &value);

    void clear();

    void dedup_unstable();
    void dedup();

    void swap(usize a, usize b);

    Option<T &> first();
    Option<T &> last();
    Option<const T &> first() const;
    Option<const T &> last() const;

    T *begin();
    T *end();
    const T *begin() const;
    const T *end() const;
    const T &operator[](usize i) const;
    T &operator[](usize i);

    _VIXEN_IMPL_SLICE_OPERATORS(T, data, length)

    usize len() const {
        return length;
    }

    operator Slice<const T>() const;
    operator Slice<T>();

    template <typename S>
    S &operator<<(S &s);

private:
    usize next_capacity();
    void try_grow(usize elements_needed);
    void set_capacity(usize cap);

private:
    Allocator *alloc = nullptr;
    T *data = nullptr;
    usize length = 0, capacity = 0;
};

template <typename T, typename H>
inline void hash(const Vector<T> &values, H &hasher);

template <typename T>
struct is_collection<Vector<T>> : std::true_type {};

template <typename T>
struct collection_types<Vector<T>> : standard_collection_types<T> {};

template <typename T,
    typename U,
    require<std::is_convertible<decltype(std::declval<T const &>() == std::declval<U const &>()),
        bool>> = true>
constexpr bool operator==(Vector<T> const &lhs, Vector<U> const &rhs) {
    if (lhs.len() != rhs.len())
        return false;

    for (usize i = 0; i < lhs.len(); ++i) {
        if (!(lhs[i] == rhs[i])) {
            return false;
        }
    }

    return true;
}

template <typename T,
    typename U,
    require<std::is_convertible<decltype(std::declval<T const &>() != std::declval<U const &>()),
        bool>> = true>
constexpr bool operator!=(Vector<T> const &lhs, Vector<U> const &rhs) {
    if (lhs.len() != rhs.len())
        return true;

    for (usize i = 0; i < lhs.len(); ++i) {
        if (!(lhs[i] != rhs[i])) {
            return false;
        }
    }

    return true;
}

} // namespace vixen

#include "vec.inl"