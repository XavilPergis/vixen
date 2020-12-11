#pragma once

#include "vixen/types.hpp"
#include "vixen/util.hpp"

namespace vixen {

template <usize Size, usize Align>
struct raw_uninitialized_storage {
    static_assert(Size > 0);
    static_assert(Align > 0);

    using type = alignas(Align) u8[Size];
};

template <usize Size, usize Align>
using raw_uninitialized_storage_type = typename raw_uninitialized_storage<Size, Align>::type;

template <typename T>
struct uninitialized_storage {
    template <typename U>
    void set(U &&new_value) {
        util::construct_in_place(reinterpret_cast<T *>(raw), std::forward<U>(new_value));
    }

    void erase() {
        get().~T();
    }

    T &get() {
        return *reinterpret_cast<T *>(raw);
    }

    T const &get() const {
        return *reinterpret_cast<const T *>(raw);
    }

    raw_uninitialized_storage_type<sizeof(T), alignof(T)> raw;
};

} // namespace vixen
