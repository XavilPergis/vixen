#pragma once

#include <xen/types.hpp>

namespace xen {

template <typename Scalar>
struct color {
    Scalar red;
    Scalar green;
    Scalar blue;
    Scalar alpha;

    bool operator==(const color &other) const {
        return red == other.red && green == other.green && blue == other.blue
            && alpha == other.alpha;
    }

    bool operator!=(const color &other) const {
        return red != other.red || green != other.green || blue != other.blue
            || alpha != other.alpha;
    }
};

} // namespace xen
