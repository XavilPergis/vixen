#pragma once

#include "vixen/assert.hpp"
#include "vixen/option.hpp"
#include "vixen/result.hpp"
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

// clang-format off
template <typename T, typename Map>
result<usize, usize> binary_search(
    const T *start,
    const T *end,
    const decltype(std::declval<const Map &>()(std::declval<const T &>())) &search_for,
    const Map &mapper
) {
    // clang-format on
    if (end == start)
        return err(0);

    isize lo = 0, hi = (end - start) - 1;
    usize mid = 0;
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        const T &mid_item = start[mid];
        if (search_for == mapper(mid_item)) {
            return ok(mid);
        } else if (search_for > mapper(mid_item)) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }

    return err(search_for > mapper(start[mid]) ? mid + 1 : mid);
}

inline option<usize> get_previous_from_binary_search_results(const result<usize, usize> &res) {
    if (res.is_ok()) {
        return res.unwrap_ok();
    } else {
        auto prev = res.unwrap_err();
        if (prev == 0)
            return empty_opt;
        return prev - 1;
    }
}

// clang-format off
template <typename T, typename Map>
option<usize> binary_find_previous_index(
    const T *start,
    const T *end,
    const decltype(std::declval<const Map &>()(std::declval<const T &>())) &search_for,
    const Map &mapper
) {
    // clang-format on
    return get_previous_from_binary_search_results(binary_search(start, end, search_for, mapper));
}

} // namespace vixen

#include "slice.inl"
