#pragma once

#include "xen/allocator/allocator.hpp"
#include "xen/option.hpp"
#include "xen/slice.hpp"

#include <xen/traits.hpp>
#include <xen/types.hpp>

namespace xen {
// Number of elements in the initial allocation of a vector.
constexpr const usize default_vec_capacity = 8;

// Growable dynamically-allocated array. Any reference into a vector is only valid until
// the vector's size changes.
template <typename T>
struct vector {
    vector();

    explicit vector(allocator *alloc);
    vector(allocator *alloc, usize default_capacity);
    vector(allocator *alloc, const vector<T> &other);
    vector(vector<T> &&other);
    vector<T> &operator=(vector<T> &&other);

    ~vector();

    vector<T> clone(allocator *alloc);

    /// Appends a single item to the end of the vector.
    template <typename U>
    void push(U &&value);

    /// Pushes `elements` elements from `ptr` onto the end of the vector.
    // void extend(Iterator<T> elements);

    /// Extends the vector by `elements` elements, and returns a pointer to the start of the
    /// newly allocated elements.
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

    _XEN_IMPL_SLICE_OPERATORS(T, data, length)

    usize len() const;

    const vector<T> &as_const() const;
    operator slice<const T>() const;
    operator slice<T>();

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

} // namespace xen

#include "inl/vec.inl"