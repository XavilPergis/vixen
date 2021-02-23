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

    template <typename... Args>
    unique(allocator *alloc, Args &&...args);
    unique(allocator *alloc, const unique &other);

    unique(unique &&other);
    unique &operator=(unique &&other);

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
        template <typename... Args>
        inner(allocator *alloc, Args &&...args) : data(std::forward<Args>(args)...), alloc(alloc) {}

        T data;
        allocator *alloc;
    };

    inner *inner_pointer = nullptr;
};

template <typename T, typename H>
inline void hash(const unique<T> &option, H &hasher);

} // namespace vixen

#include "unique.inl"
