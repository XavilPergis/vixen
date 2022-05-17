#pragma once

#include "vixen/types.hpp"
#include "vixen/util.hpp"

namespace vixen {

template <typename T>
struct UninitializedStorage {
    template <typename... Args>
    constexpr void constructInPlace(Args &&...args) {
        util::constructInPlace(reinterpret_cast<T *>(raw), std::forward<Args...>(args)...);
    }

    constexpr void destruct() { get().~T(); }

    constexpr T &get() { return *reinterpret_cast<T *>(raw); }
    constexpr T const &get() const { return *reinterpret_cast<const T *>(raw); }
    constexpr T *data() { return reinterpret_cast<T *>(raw); }
    constexpr T const *data() const { return reinterpret_cast<const T *>(raw); }

    alignas(T) opaque raw[sizeof(T)];
};

} // namespace vixen
