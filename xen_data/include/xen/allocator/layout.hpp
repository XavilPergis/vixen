#pragma once

#include <xen/types.hpp>

namespace xen::heap {

struct layout {
    bool operator==(const layout &rhs) const;
    bool operator!=(const layout &rhs) const;

    /// Create a layout which describes the size and alignment characteristics of `T`.
    template <typename T>
    static constexpr layout of();

    /// Like `layout::of`, except repeated `len` times.
    template <typename T>
    static constexpr layout array_of(usize len);

    /// Creates a layout that describes an array of the current layout, repeated `len` times.
    constexpr layout array(usize len) const;

    /// Creates a layout with its size set to `size` and its alignment set to the current alignment.
    constexpr layout with_size(usize size) const;

    /// Creates a layout with its alignment set to `align` and its size set to the current size.
    constexpr layout with_align(usize align) const;

    /// Creates a layout with its alignment large enough to satisfy both the current layout and the
    /// new alignment.
    constexpr layout add_alignment(usize align) const;

    usize size, align;

    template <typename S>
    friend S &operator<<(S &s, const layout &layout) {
        return s << layout.size << "b:" << layout.align << "a";
    }
};

} // namespace xen::heap
