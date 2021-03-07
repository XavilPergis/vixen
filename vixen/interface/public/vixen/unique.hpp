#pragma once

#include "vixen/allocator/allocator.hpp"

namespace vixen {

/// @ingroup vixen_data_structures
/// @brief Managed pointer that destroys its allocation at the end of its lifetime.
template <typename T>
struct Unique {
    using pointer = T *;
    using const_pointer = const T *;

    Unique() = default;

    template <typename... Args>
    Unique(Allocator *alloc, Args &&...args);
    Unique(Allocator *alloc, const Unique &other);

    Unique(Unique &&other);
    Unique &operator=(Unique &&other);

    ~Unique();

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
        template <typename... Args>
        inner(Allocator *alloc, Args &&...args) : data(std::forward<Args>(args)...), alloc(alloc) {}

        T data;
        Allocator *alloc;
    };

    inner *inner_pointer = nullptr;
};

template <typename T, typename H>
inline void hash(const Unique<T> &option, H &hasher);

} // namespace vixen

#include "unique.inl"
