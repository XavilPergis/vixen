#pragma once

#include "vixen/assert.hpp"
#include "vixen/option.hpp"
#include "vixen/result.hpp"
#include "vixen/types.hpp"
#include "vixen/util.hpp"

#pragma region "slice Operator Macro"
#define _VIXEN_IMPL_SLICE_OPERATORS(Ty, ptr, len)                          \
    View<const Ty> operator[](Range range) const noexcept {                \
        _VIXEN_BOUNDS_CHECK(range.start, len);                             \
        _VIXEN_BOUNDS_CHECK_EXCLUSIVE(range.end, len);                     \
        return View<const Ty>{ptr + range.start, range.end - range.start}; \
    }                                                                      \
    View<const Ty> operator[](RangeFrom range) const noexcept {            \
        _VIXEN_BOUNDS_CHECK(range.start, len);                             \
        return View<const Ty>{ptr + range.start, len - range.start};       \
    }                                                                      \
    View<const Ty> operator[](RangeTo range) const noexcept {              \
        _VIXEN_BOUNDS_CHECK_EXCLUSIVE(range.end, len);                     \
        return View<const Ty>{ptr, range.end};                             \
    }                                                                      \
    View<T> operator[](Range range) noexcept {                             \
        _VIXEN_BOUNDS_CHECK(range.start, len);                             \
        _VIXEN_BOUNDS_CHECK_EXCLUSIVE(range.end, len);                     \
        return View<Ty>{ptr + range.start, range.end - range.start};       \
    }                                                                      \
    View<T> operator[](RangeFrom range) noexcept {                         \
        _VIXEN_BOUNDS_CHECK(range.start, len);                             \
        return View<Ty>{ptr + range.start, len - range.start};             \
    }                                                                      \
    View<T> operator[](RangeTo range) noexcept {                           \
        _VIXEN_BOUNDS_CHECK_EXCLUSIVE(range.end, len);                     \
        return View<Ty>{ptr, range.end};                                   \
    }
#pragma endregion

namespace vixen {
struct Range {
    usize start, end;
};

constexpr Range range(usize start, usize end) noexcept {
    return {start, end};
}

struct RangeFrom {
    usize start;
};

constexpr RangeFrom rangeFrom(usize start) noexcept {
    return {start};
}

struct RangeTo {
    usize end;
};

constexpr RangeTo rangeTo(usize end) noexcept {
    return {end};
}

template <typename T>
struct View {
    View() = default;
    VIXEN_DEFAULT_ALL(View)

    constexpr View(T *start, usize len) noexcept : mPtr(start), mLength(len) {}

    constexpr static View fromParts(T *start, usize len) noexcept { return View(start, len); }

    /**
     * @brief Construct a view from a pointer to the start, and a pointer to one element past the
     * end.
     *
     * @warning both start and end must have the same provenance!
     */
    constexpr static View fromPointerRange(T *start, T *end) noexcept {
        return View(start, start - end);
    }

    constexpr operator View<const T>() const noexcept { return View<const T>{mPtr, mLength}; }

    _VIXEN_IMPL_SLICE_OPERATORS(T, mPtr, mLength)

    constexpr T &operator[](usize i) noexcept {
        _VIXEN_BOUNDS_CHECK(i, mLength);
        return mPtr[i];
    }
    constexpr const T &operator[](usize i) const noexcept {
        _VIXEN_BOUNDS_CHECK(i, mLength);
        return mPtr[i];
    }

    constexpr Option<T &> first() noexcept { return isEmpty() ? EMPTY_OPTION : mPtr[0]; }
    constexpr Option<T &> last() noexcept { return isEmpty() ? EMPTY_OPTION : mPtr[mLength - 1]; }
    constexpr Option<const T &> first() const noexcept {
        return isEmpty() ? EMPTY_OPTION : mPtr[0];
    }
    constexpr Option<const T &> last() const noexcept {
        return isEmpty() ? EMPTY_OPTION : mPtr[mLength - 1];
    }

    constexpr T *begin() noexcept { return mPtr; }
    constexpr T *end() noexcept { return mPtr + mLength; }
    constexpr const T *begin() const noexcept { return mPtr; }
    constexpr const T *end() const noexcept { return mPtr + mLength; }

    constexpr usize len() const noexcept { return mLength; }
    constexpr bool isEmpty() const noexcept { return mLength == 0; }

private:
    T *mPtr = nullptr;
    usize mLength = 0;
};

template <typename T>
using ImmutableView = View<const T>;

template <typename T, typename U>
inline bool operator==(View<const T> const &lhs, View<const U> const &rhs) noexcept {
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
inline bool operator!=(View<const T> const &lhs, View<const U> const &rhs) noexcept {
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
inline void hash(View<T> const &values, H &hasher) noexcept {
    for (T const &value : values) {
        hash(value, hasher);
    }
}
template <typename T>
struct is_collection<View<T>> : std::true_type {};

template <typename T>
struct collection_types<View<T>> : standard_collection_types<T> {};

// clang-format off
template <typename T, typename Map>
result<usize, usize> binary_search(
    const T *start,
    const T *end,
    const decltype(std::declval<const Map &>()(std::declval<const T &>())) &search_for,
    const Map &mapper
) {
    // clang-format on
    if (end == start) return err(0);

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
        if (prev == 0) return EMPTY_OPTION;
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
