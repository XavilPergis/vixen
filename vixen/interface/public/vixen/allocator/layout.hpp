#pragma once

#include "vixen/types.hpp"

/// @file
/// @ingroup vixen_allocator

namespace vixen::heap {

struct Layout {
    bool operator==(const Layout &rhs) const;
    bool operator!=(const Layout &rhs) const;

    /// Create a layout which describes the size and alignment characteristics of `T`.
    template <typename T>
    static constexpr Layout of();

    /// Like `layout::of`, except repeated `len` times.
    template <typename T>
    static constexpr Layout array_of(usize len);

    /// Creates a layout that describes an array of the current layout, repeated `len` times.
    constexpr Layout array(usize len) const;

    /// Creates a layout with its size set to `size` and its alignment set to the current alignment.
    constexpr Layout with_size(usize size) const;

    /// Creates a layout with its alignment set to `align` and its size set to the current size.
    constexpr Layout with_align(usize align) const;

    /// Creates a layout with its alignment large enough to satisfy both the current layout and the
    /// new alignment.
    constexpr Layout add_alignment(usize align) const;

    constexpr Layout packed_duplicate(usize count) const {
        return {count * size, align};
    }

    usize size, align;

    template <typename S>
    friend S &operator<<(S &s, const Layout &layout) {
        return s << layout.size << "b:" << layout.align << "a";
    }
};

} // namespace vixen::heap
