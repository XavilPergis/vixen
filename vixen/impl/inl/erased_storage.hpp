#pragma once

#include "vixen/types.hpp"
#include "vixen/util.hpp"

namespace vixen {

template <usize Size, usize Align>
struct raw_uninitialized_storage {
    static_assert(Size > 0);
    static_assert(Align > 0);

    struct type {
        alignas(Align) u8 data[Size];
    };
};

template <usize Size, usize Align>
using raw_uninitialized_storage_type = typename raw_uninitialized_storage<Size, Align>::type;

template <typename T>
struct uninitialized_storage {
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
