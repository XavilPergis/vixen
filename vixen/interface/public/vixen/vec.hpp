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
        setCapacity(default_capacity);
    }

    VIXEN_DEFINE_CLONE_METHOD(Vector);

    /**
     * @brief inserts each value to the end of the vector
     *
     * @tparam Us the argument types
     * @param values a pack of items to be added to the vector. earlier values get inserted first
     */
    template <typename... Us>
    void push(Us &&...values);

    /**
     * @brief reserves one new slot and constructs a new element in-place, forwarding `values` to
     * the element's constructor
     *
     * @tparam Args the constructor's argument types
     * @param values the arguments to be forwarded to the constructor
     */
    template <typename... Args>
    void emplace(Args &&...args);

    /**
     * @brief reserves `elements` new slots
     * @warning this method does _not_ initialize any of the elements, so great care must be taken
     * to initialize them all before any exception can be thrown
     *
     * @param elements the number of new slots to reserve at the end of the vector
     * @return T* a pointer to to the newly allocated slots
     */
    T *reserve(usize elements = 1);

    /**
     * @brief truncates the vector to `len` elements
     * @note it is a logic error to truncate a vector to a length greater than its current length
     *
     * @param len the new length of the vector
     */
    void truncate(usize len);

    /**
     * @brief removes an element from the end of the vector
     *
     * @return Option<T> the removed element, or an empty option if the vector was empty
     */
    Option<T> pop();

    /**
     * @brief removes the element at index `idx`
     * @warning this operation does not preserve ordering
     *
     * this method removes elements by swapping the last element and the element at `idx`, and then
     * popping the last element. this does not preserve the order of the elements in the vector, but
     * allows the removal to be constant time.
     *
     * @param idx the index of the element to remove
     * @return T the removed element
     */
    T remove(usize idx);

    /**
     * @brief removes the element at index `idx`
     *
     * this method removes elements by shifting all the elements after `idx` down by one, and
     * lowering the length by one. this means the operation preserves the order of elements in the
     * vector, but at the cost of being linear in time with the number of elements in the vector.
     *
     * @param idx
     * @return T
     */
    T shiftRemove(usize idx);

    /**
     * @brief inserts each value in sequence
     *
     * @tparam Us
     * @param idx
     * @param vals
     * @return T*
     */
    template <typename... Us>
    T *insert(usize idx, Us &&...vals);

    template <typename... Us>
    T *shiftInsert(usize idx, Us &&...vals);

    template <typename... Args>
    T &emplaceInsert(usize idx, Args &&...args);

    template <typename... Args>
    T &emplaceShiftInsert(usize idx, Args &&...args);

    Option<usize> firstIndexOf(const T &value);
    Option<usize> lastIndexOf(const T &value);

    void clear();

    void dedupUnstable();
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
    usize nextCapacity();
    void ensureCapacity(usize elements_needed);
    void setCapacity(usize cap);

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