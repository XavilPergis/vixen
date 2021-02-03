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
struct vector {
    /// Default construction of a vector results in a vector in the moved-from state.
    vector();

    explicit vector(allocator *alloc);
    vector(allocator *alloc, usize default_capacity);
    vector(allocator *alloc, const vector<T> &other);
    vector(vector<T> &&other);
    vector<T> &operator=(vector<T> &&other);

    ~vector();

    vector<T> clone(allocator *alloc) const;

    /// Appends a single item to the end of the vector.
    template <typename U>
    void push(U &&value);

    /// Extends the vector by `elements` elements, and returns a pointer to the start of the
    /// newly allocated elements.
    ///
    /// @warning This method does _not_ initialize any elements.
    T *reserve(usize elements = 1);

    /// Sets the length of the vector to `len` items long.
    void truncate(usize len);

    /// Removes the last item from the vector and returns it, or en empty option if there is nothing
    /// to pop.
    option<T> pop();

    /// Removes an item from the list efficiently by swapping it with the last element and
    /// popping. This operation does not preserve ordering. O(1)
    T remove(usize idx);

    /// Removes an item for the list by shifting all elements past `idx` back one. O(n)
    T shift_remove(usize idx);

    template <typename U>
    T &shift_insert(usize idx, U &&val);

    option<usize> index_of(const T &value);

    void clear();

    void dedup_unstable();
    void dedup();

    void swap(usize a, usize b);

    option<T &> first();
    option<T &> last();
    option<const T &> first() const;
    option<const T &> last() const;

    T *begin();
    T *end();
    const T *begin() const;
    const T *end() const;
    const T &operator[](usize i) const;
    T &operator[](usize i);

    _VIXEN_IMPL_SLICE_OPERATORS(T, data, length)

    usize len() const;

    const vector<T> &as_const() const;
    operator slice<const T>() const;
    operator slice<T>();

    template <typename S>
    S &operator<<(S &s);

private:
    usize next_capacity();
    void try_grow(usize elements_needed);
    void set_capacity(usize cap);

private:
    allocator *alloc = nullptr;
    T *data = nullptr;
    usize length = 0, capacity = 0;
};

template <typename T, typename H>
inline void hash(const vector<T> &values, H &hasher);

template <typename T>
struct is_collection<vector<T>> : std::true_type {};

template <typename T>
struct collection_types<vector<T>> : standard_collection_types<T> {};

} // namespace vixen

#include "vec.inl"