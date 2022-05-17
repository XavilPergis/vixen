#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/option.hpp"
#include "vixen/slice.hpp"
#include "vixen/traits.hpp"
#include "vixen/types.hpp"

#include <iterator>

/// @defgroup vixen_data_structures Data Structures

namespace vixen {
/// Number of elements in the initial allocation of a vector.
constexpr const usize DEFAULT_VEC_CAPACITY = 8;

template <typename T>
struct VectorOutputIterator;

/**
 * @brief growable dynamically-allocated array
 * @warning any reference into a vector is only valid until the vector's size changes
 *
 * @tparam T the element type
 * @ingroup vixen_data_structures
 */
template <typename T>
struct Vector {
    static_assert(std::is_nothrow_destructible_v<T>);
    static_assert(std::is_nothrow_move_constructible_v<T>);
    static_assert(std::is_nothrow_move_assignable_v<T>);

    /// default construction of a vector results in a vector in the moved-from state
    Vector() = default;

    VIXEN_DELETE_COPY(Vector)

    Vector(Vector<T> &&other) noexcept;
    Vector<T> &operator=(Vector<T> &&other) noexcept;

    ~Vector();

    // Allocator-aware copy ctor
    Vector(copy_tag_t, Allocator &alloc, Vector<T> const &other);
    VIXEN_DEFINE_CLONE_METHOD(Vector)

    explicit Vector(Allocator &alloc) noexcept : mAlloc(&alloc) {}
    Vector(Allocator &alloc, usize defaultCapacity) : mAlloc(&alloc) {
        setCapacity(defaultCapacity);
    }

    VIXEN_NODISCARD static Vector empty(Allocator &alloc) noexcept { return Vector(alloc); }
    VIXEN_NODISCARD static Vector withCapacity(Allocator &alloc, usize defaultCapacity) {
        return Vector(alloc, defaultCapacity);
    }

    /**
     * @brief reserves the space for `elements` new elements after the last element in the current
     * vector, but does not insert them.
     *
     * @param elements the number of elements to reserve space for.
     * @return T* a pointer to to the beginning of the newly-allocated elements.
     */
    T *reserveLast(usize additionalElements = 1);

    /**
     * @brief increases the length of the vector by `elements`.
     *
     * @pre the current length + `elements` must be less than or equal to the capacity of the
     * vector.
     * @pre the newly-added elements must already be initialized.
     *
     * @param elements
     */
    void unsafeGrowBy(usize elements) noexcept;

    // growTo - not included, seems like it would not be very useful...

    /**
     * @brief decreases the length of the vector by `elements`.
     *
     * @pre the current length - `elements` must be greater than or equal to `0`.
     *
     * @param elements
     */
    void shrinkBy(usize elements) noexcept;

    void shrinkByNoDestroy(usize elements) noexcept;

    /**
     * @brief truncates the vector to `len` elements.
     *
     * @note it is a logic error to truncate a vector to a length greater than its current length.
     *
     * @param length the new length of the vector
     */
    void shrinkTo(usize length) noexcept;

    void shrinkToNoDestroy(usize length) noexcept;

    /**
     * @brief sets the length of the vector to `len` elements.
     *
     * @pre The new length must be greater than or equal to the capacity of this vector
     * @pre If the new length is greater than the previous length, the newly-added elements must
     * already be initialized.
     *
     * @param length the new length of the vector
     */
    void unsafeSetLength(usize length) noexcept;

    void unsafeSetLengthNoDestroy(usize length) noexcept;

#pragma region "Insertion"

    // IDEA: maybe rename `push` to `extend` and `emplace` to `push`.
    // could also maybe do `insertLast` and `emplaceLast` or `insertEmplaceLast`

    /**
     * @brief inserts each value in sequence after the last element in the current vector.
     *
     * Satisfies the basic exception safety guarantees. In the case an exception is thrown, the only
     * state affected is the capacity of the vector.
     */
    template <typename... Us>
    void insertLast(Us &&...values);

    /**
     * @brief inserts a new element after the last element in the current vector.
     *
     * This method differs from `push` in that this will construct a single new element by
     * forwarding the provided arguments to `T`'s constructor, instead of invoking `T`'s move
     * constructor multiple times like `push` does.
     *
     * Satisfies the basic exception safety guarantees. In the case an exception is thrown, the only
     * state affected is the capacity of the vector.
     */
    template <typename... Args>
    void emplaceLast(Args &&...args);

    /**
     * @brief inserts each value in sequence, starting at index `idx`
     * @warning this operation does not preserve ordering
     *
     * this method acts as if you had invoked `emplaceSwap` for every passed parameter.
     *
     * Time Complexity: [amoritized] `O(1)`.
     *
     * Satisfies the basic exception safety guarantees.
     *
     * @param idx the index to insert before, in the range `[0, length]`. an index of `0` will
     * insert the new elements before the first element of the current vector, and an index of
     * `length` will insert the new elements after the last element of the current vector.
     * @param values the values to insert
     * @return View<T> a view of the newly-inserted elements
     */
    template <typename... Us>
    View<T> insertSwap(usize idx, Us &&...values);

    template <typename... Args>
    T &emplaceSwap(usize idx, Args &&...args);

    /**
     * @brief inserts each value in sequence, starting at index `idx`
     *
     * this method inserts elements by shifting all subsequent elements down enough to fit the new
     * elements, which are constructed in-place. this means the operation preserves the order of
     * elements in the vector, but at the cost of being linear in time with the number of elements
     * in the vector.
     *
     * Satisfies the basic exception safety guarantees.
     *
     * @param idx the index to insert before, in the range `[0, length]`. an index of `0` will
     * insert the new elements before the first element of the current vector, and an index of
     * `length` will insert the new elements after the last element of the current vector.
     * @param values the values to insert
     * @return View<T> a view of the newly-inserted elements
     */
    template <typename... Us>
    View<T> insertShift(usize idx, Us &&...values);

    template <typename... Args>
    T &emplaceShift(usize idx, Args &&...args);

#pragma endregion
#pragma region "Removal"

    /**
     * @brief removes an element from the end of the vector
     *
     * @pre the vector must not be empty.
     *
     * @return Option<T> the removed element, or an empty option if the vector was empty
     */
    T removeLast() noexcept;

    /**
     * @brief removes the element at index `idx`
     * @warning this operation does not preserve ordering
     *
     * @pre `idx` must be less than the length of the vector.
     *
     * this method removes elements by swapping the last element and the element at `idx`, and then
     * popping the last element. this does not preserve the order of the elements in the vector, but
     * allows the removal to be constant time.
     *
     * @param idx the index of the element to remove
     * @return the removed element
     */
    T removeSwap(usize idx) noexcept;

    /**
     * @brief removes the element at index `idx`
     *
     * @pre `idx` must be less than the length of the vector.
     *
     * this method removes elements by shifting all the elements after `idx` down by one, and
     * lowering the length by one. this means the operation preserves the order of elements in the
     * vector, but at the cost of being linear in time with the number of elements in the vector.
     *
     * @param idx the index of the element to remove
     * @return the removed element
     */
    T removeShift(usize idx) noexcept;

    /**
     * @brief removes all elements fro the vector.
     */
    void removeAll() noexcept;

#pragma endregion

    VIXEN_NODISCARD bool contains(T const &value) noexcept;

    // void sortStable();
    // void sortUnstable();

    VIXEN_NODISCARD Option<usize> firstIndexOf(T const &value) noexcept;
    VIXEN_NODISCARD Option<usize> lastIndexOf(T const &value) noexcept;

    void dedupUnstable() noexcept;
    void dedup() noexcept;

    void swap(usize a, usize b) noexcept;

    VIXEN_NODISCARD VectorOutputIterator<T> insertLastIterator() noexcept;

    // clang-format off
    VIXEN_NODISCARD usize len() const noexcept { return mLength; }
    VIXEN_NODISCARD usize capacity() const noexcept { return mCapacity; }
    VIXEN_NODISCARD bool isEmpty() const noexcept { return mLength == 0; }
    // clang-format on

    // clang-format off
    VIXEN_NODISCARD T &first() noexcept { return mData[0]; }
    VIXEN_NODISCARD T &last() noexcept { return mData[mLength - 1]; }
    VIXEN_NODISCARD const T &first() const noexcept { return mData[0]; }
    VIXEN_NODISCARD const T &last() const noexcept { return mData[mLength - 1]; }

    VIXEN_NODISCARD T *begin() noexcept { return mData; }
    VIXEN_NODISCARD T *end() noexcept { return mData + mLength; }
    VIXEN_NODISCARD T const *begin() const noexcept { return mData; }
    VIXEN_NODISCARD T const *end() const noexcept { return mData + mLength; }
    // clang-format on

    VIXEN_NODISCARD T const &operator[](usize i) const noexcept;
    VIXEN_NODISCARD T &operator[](usize i) noexcept;

    _VIXEN_IMPL_SLICE_OPERATORS(T, mData, mLength)

    operator View<const T>() const noexcept;
    operator View<T>() noexcept;

    template <typename S>
    S &operator<<(S &s);

private:
    usize nextCapacity() noexcept;
    void ensureCapacity(usize elementsNeeded);
    void setCapacity(usize cap);

    // just a convenience for `addressof(mData[index])`
    constexpr T *ptrTo(usize index) {
        return std::addressof(mData[index]);
    }
    constexpr T const *ptrTo(usize index) const {
        return std::addressof(mData[index]);
    }

private:
    // IDEA: maybe store capacity and allocator out-of-line? i suspect they are relatively cold
    // compared to length and data accesses.
    Allocator *mAlloc = nullptr;
    T *mData = nullptr;
    usize mLength = 0, mCapacity = 0;
};

template <typename T>
struct VectorOutputIterator
    : public std::iterator<std::output_iterator_tag, void, void, void, void> {
    explicit VectorOutputIterator(Vector<T> *output) : output(output) {}

    // i hate this
    VectorOutputIterator(VectorOutputIterator const &other) : output(other.output) {}
    VectorOutputIterator &operator=(VectorOutputIterator const &other) {
        output = other.output;
        return *this;
    }

    VectorOutputIterator &operator=(T &&value) {
        output->insertLast(mv(value));
        return *this;
    }

    VectorOutputIterator &operator=(T const &value) {
        output->insertLast(value);
        return *this;
    }

    VectorOutputIterator &operator*() { return *this; }
    VectorOutputIterator &operator++() { return *this; }
    VectorOutputIterator &operator++(int) { return *this; }

private:
    Vector<T> *output;
};

template <typename T>
VectorOutputIterator<T> Vector<T>::insertLastIterator() noexcept {
    return VectorOutputIterator<T>{this};
}

// template <typename T>
// struct HashTraits<Vector<T>> {
//     template <typename H>
//     static void hash(Vector<T> const &values, H &hasher);
// };

template <typename T, typename H>
inline void hash(Vector<T> const &values, H &hasher);

template <typename T>
struct is_collection<Vector<T>> : std::true_type {};

template <typename T>
struct collection_types<Vector<T>> : standard_collection_types<T> {};

// template <typename T>
// struct ContainerTraits<Vector<T>> {
//     using Container = Vector<T>;
//     using Contained = T;

//     static usize elementCount(Vector<T> const &vector) { return vector.len(); }

//     // T = char
//     template <typename U>
//     static void insertLast(Vector<T> &vector, U &&item) {
//         vector.insertLast(std::forward<U>(item));
//     }
// };

template <typename T,
    typename U,
    require<std::is_convertible<decltype(std::declval<T const &>() == std::declval<U const &>()),
        bool>> = true>
constexpr bool operator==(Vector<T> const &lhs, Vector<U> const &rhs) noexcept {
    if (lhs.len() != rhs.len()) return false;

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
constexpr bool operator!=(Vector<T> const &lhs, Vector<U> const &rhs) noexcept {
    if (lhs.len() != rhs.len()) return true;

    for (usize i = 0; i < lhs.len(); ++i) {
        if (!(lhs[i] != rhs[i])) {
            return false;
        }
    }

    return true;
}

} // namespace vixen

#include "vector.inl"