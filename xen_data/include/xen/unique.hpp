#pragma once

#include <xen/allocator/allocator.hpp>

namespace xen {

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

private:
    allocator *alloc;
    T *data;
};

template <typename T, typename H>
inline void hash(const unique<T> &option, H &hasher);

} // namespace xen

#include "inl/unique.inl"
