#pragma once

#include "vixen/allocator/allocator.hpp"

namespace vixen {

/// @ingroup vixen_data_structures
/// @brief Managed pointer that destroys its allocation at the end of its lifetime.
template <typename T>
struct unique {
    using pointer = T *;
    using const_pointer = const T *;

    unique() = default;

    template <typename U>
    unique(allocator *alloc, U &&other);
    unique(allocator *alloc, const unique<T> &other);

    unique(unique<T> &&other);
    unique<T> &operator=(unique<T> &&other);

    ~unique();

    const T &operator*() const;
    const T *operator->() const;
    T &operator*();
    T *operator->();

    explicit operator bool() const;
    operator const_pointer() const;
    operator pointer();

    void clear();

    template <typename S>
    S &operator<<(S &s) const {
        s << **this;
        return s;
    }

private:
    struct inner {
        T data;
        allocator *alloc;
    };

    inner *inner_pointer = nullptr;
};

template <typename T, typename H>
inline void hash(const unique<T> &option, H &hasher);

} // namespace vixen

#include "unique.inl"
