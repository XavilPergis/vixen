#pragma once

#include "vixen/assert.hpp"
#include "vixen/option.hpp"
#include "vixen/types.hpp"
#include "vixen/util.hpp"

#pragma region "slice Operator Macro"
#define _VIXEN_IMPL_SLICE_OPERATORS(Ty, ptr, len)                           \
    slice<const Ty> operator[](range range) const {                         \
        _VIXEN_BOUNDS_CHECK(range.start, len);                              \
        _VIXEN_BOUNDS_CHECK_EXCLUSIVE(range.end, len);                      \
        return slice<const Ty>{ptr + range.start, range.end - range.start}; \
    }                                                                       \
    slice<const Ty> operator[](range_from range) const {                    \
        _VIXEN_BOUNDS_CHECK(range.start, len);                              \
        return slice<const Ty>{ptr + range.start, len - range.start};       \
    }                                                                       \
    slice<const Ty> operator[](range_to range) const {                      \
        _VIXEN_BOUNDS_CHECK_EXCLUSIVE(range.end, len);                      \
        return slice<const Ty>{ptr, range.end};                             \
    }                                                                       \
    slice<T> operator[](range range) {                                      \
        _VIXEN_BOUNDS_CHECK(range.start, len);                              \
        _VIXEN_BOUNDS_CHECK_EXCLUSIVE(range.end, len);                      \
        return slice<Ty>{ptr + range.start, range.end - range.start};       \
    }                                                                       \
    slice<T> operator[](range_from range) {                                 \
        _VIXEN_BOUNDS_CHECK(range.start, len);                              \
        return slice<Ty>{ptr + range.start, len - range.start};             \
    }                                                                       \
    slice<T> operator[](range_to range) {                                   \
        _VIXEN_BOUNDS_CHECK_EXCLUSIVE(range.end, len);                      \
        return slice<Ty>{ptr, range.end};                                   \
    }
#pragma endregion

namespace vixen {
struct range {
    usize start, end;

    range(usize start, usize end) : start(start), end(end) {}
};

struct range_from {
    usize start;

    range_from(usize start) : start(start) {}
};

struct range_to {
    usize end;

    range_to(usize end) : end(end) {}
};

template <typename T>
struct slice {
    T *ptr;
    usize len;

    slice() = default;

    operator slice<const T>() const;
    const slice<T> &as_const() const;

    _VIXEN_IMPL_SLICE_OPERATORS(T, ptr, len)
    T &operator[](usize i);
    const T &operator[](usize i) const;

    option<T &> first();
    option<T &> last();
    option<const T &> first() const;
    option<const T &> last() const;

    T *begin();
    T *end();
    const T *begin() const;
    const T *end() const;
};

template <typename T>
inline bool operator==(slice<const T> const &lhs, slice<const T> const &rhs);

template <typename T, typename H>
inline void hash(const slice<T> &values, H &hasher);

template <typename T>
struct is_collection<slice<T>> : std::true_type {};

template <typename T>
struct collection_types<slice<T>> : standard_collection_types<T> {};

} // namespace vixen

#include "slice.inl"
