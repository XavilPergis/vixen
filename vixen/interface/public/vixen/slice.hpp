#pragma once

#include "vixen/assert.hpp"
#include "vixen/option.hpp"
#include "vixen/result.hpp"
#include "vixen/types.hpp"
#include "vixen/util.hpp"

#pragma region "slice Operator Macro"
#define _VIXEN_IMPL_SLICE_OPERATORS(Ty, ptr, len)                           \
    Slice<const Ty> operator[](Range range) const {                         \
        _VIXEN_BOUNDS_CHECK(range.start, len);                              \
        _VIXEN_BOUNDS_CHECK_EXCLUSIVE(range.end, len);                      \
        return Slice<const Ty>{ptr + range.start, range.end - range.start}; \
    }                                                                       \
    Slice<const Ty> operator[](RangeFrom range) const {                     \
        _VIXEN_BOUNDS_CHECK(range.start, len);                              \
        return Slice<const Ty>{ptr + range.start, len - range.start};       \
    }                                                                       \
    Slice<const Ty> operator[](RangeTo range) const {                       \
        _VIXEN_BOUNDS_CHECK_EXCLUSIVE(range.end, len);                      \
        return Slice<const Ty>{ptr, range.end};                             \
    }                                                                       \
    Slice<T> operator[](Range range) {                                      \
        _VIXEN_BOUNDS_CHECK(range.start, len);                              \
        _VIXEN_BOUNDS_CHECK_EXCLUSIVE(range.end, len);                      \
        return Slice<Ty>{ptr + range.start, range.end - range.start};       \
    }                                                                       \
    Slice<T> operator[](RangeFrom range) {                                  \
        _VIXEN_BOUNDS_CHECK(range.start, len);                              \
        return Slice<Ty>{ptr + range.start, len - range.start};             \
    }                                                                       \
    Slice<T> operator[](RangeTo range) {                                    \
        _VIXEN_BOUNDS_CHECK_EXCLUSIVE(range.end, len);                      \
        return Slice<Ty>{ptr, range.end};                                   \
    }
#pragma endregion

namespace vixen {
struct Range {
    usize start, end;
};

constexpr Range range(usize start, usize end) {
    return {start, end};
}

struct RangeFrom {
    usize start;
};

constexpr RangeFrom range_from(usize start) {
    return {start};
}

struct RangeTo {
    usize end;
};

constexpr RangeTo range_to(usize end) {
    return {end};
}

template <typename T>
struct Slice {
    Slice() = default;
    Slice(Slice const &) = default;
    Slice &operator=(Slice const &) = default;
    Slice(Slice &&) = default;
    Slice &operator=(Slice &&) = default;

    constexpr Slice(T *start, T *end) : ptr(start), length(end - start) {}
    constexpr Slice(T *start, usize len) : ptr(start), length(len) {}

    constexpr operator Slice<const T>() const {
        return Slice<const T>{ptr, length};
    }

    _VIXEN_IMPL_SLICE_OPERATORS(T, ptr, length)
    constexpr T &operator[](usize i) {
        _VIXEN_BOUNDS_CHECK(i, length);
        return ptr[i];
    }
    constexpr const T &operator[](usize i) const {
        _VIXEN_BOUNDS_CHECK(i, length);
        return ptr[i];
    }

    constexpr Option<T &> first() {
        return length == 0 ? empty_opt : ptr[0];
    }
    constexpr Option<T &> last() {
        return length == 0 ? empty_opt : ptr[length - 1];
    }
    constexpr Option<const T &> first() const {
        return length == 0 ? empty_opt : ptr[0];
    }
    constexpr Option<const T &> last() const {
        return length == 0 ? empty_opt : ptr[length - 1];
    }

    // clang-format off
    constexpr T *begin() { return ptr; }
    constexpr T *end() { return ptr + length; }
    constexpr const T *begin() const { return ptr; }
    constexpr const T *end() const { return ptr + length; }
    constexpr usize len() const { return length; }
    // clang-format on

private:
    T *ptr = nullptr;
    usize length = 0;
};

template <typename T, typename U>
inline bool operator==(Slice<const T> const &lhs, Slice<const U> const &rhs) {
    if (lhs.len() != rhs.len()) {
        return false;
    }
    for (usize i = 0; i < lhs.len(); ++i) {
        if (!(lhs[i] == rhs[i])) {
            return false;
        }
    }
    return true;
}

template <typename T, typename U>
inline bool operator!=(Slice<const T> const &lhs, Slice<const U> const &rhs) {
    if (lhs.len() != rhs.len()) {
        return true;
    }
    for (usize i = 0; i < lhs.len(); ++i) {
        if (!(lhs[i] != rhs[i])) {
            return false;
        }
    }
    return true;
}

template <typename T, typename H>
inline void hash(Slice<T> const &values, H &hasher) {
    for (T const &value : values) {
        hash(value, hasher);
    }
}
template <typename T>
struct is_collection<Slice<T>> : std::true_type {};

template <typename T>
struct collection_types<Slice<T>> : standard_collection_types<T> {};

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

inline Option<usize> get_previous_from_binary_search_results(const result<usize, usize> &res) {
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
Option<usize> binary_find_previous_index(
    const T *start,
    const T *end,
    const decltype(std::declval<const Map &>()(std::declval<const T &>())) &search_for,
    const Map &mapper
) {
    // clang-format on
    return get_previous_from_binary_search_results(binary_search(start, end, search_for, mapper));
}

} // namespace vixen
