#pragma once

#include "vixen/types.hpp"
#include "vixen/util.hpp"

namespace vixen {

template <typename T>
struct UninitializedStorage {
    template <typename... Args>
    constexpr void construct_in_place(Args &&...args) {
        util::construct_in_place(reinterpret_cast<T *>(raw), std::forward<Args...>(args)...);
    }

    constexpr void destruct() {
        get().~T();
    }

    constexpr T &get() {
        return *reinterpret_cast<T *>(raw);
    }

    constexpr T const &get() const {
        return *reinterpret_cast<const T *>(raw);
    }

    alignas(T) u8 raw[sizeof(T)];
};

} // namespace vixen
