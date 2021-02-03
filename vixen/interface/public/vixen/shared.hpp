#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/option.hpp"
#include "vixen/types.hpp"

#include <atomic>

namespace vixen {

// SAFETY: this struct should only be used behind a pointer that was allocated via the allocator
// referred to by `alloc`.
template <typename T>
struct shared_repr {
    std::atomic<usize> strong_count;
    std::atomic<usize> weak_count;

    allocator *alloc;
    T *data;

    template <typename... Args>
    shared_repr(allocator *alloc, Args &&...args);

    void acquire_strong();
    void release_strong();
    void acquire_weak();
    void release_weak();

    bool upgrade_weak();
};

template <typename T>
struct weak;

/// @ingroup vixen_data_structures
/// @brief Managed pointer that destroys its allocation when all other shared pointers to the same
/// object are destroyed.
template <typename T>
struct shared {
    using pointer = T *;
    using const_pointer = const T *;

    template <typename... Args>
    shared(allocator *alloc, Args &&...args);

    shared() = default;
    shared(shared<T> &&other);
    shared<T> &operator=(shared<T> &&other);
    ~shared();

    // clang-format off
    const T &operator*() const { return *data; }
    const T *operator->() const { return data; }
    T &operator*() { return *data; }
    T *operator->() { return data; }

    explicit operator bool() const { return data; }
    operator const_pointer() const { return data; }
    operator pointer() { return data; }
    // clang-format on

    void clear();
    shared<T> copy();
    weak<T> downgrade();

    template <typename S>
    S &operator<<(S &s) const {
        s << **this;
        return s;
    }

private:
    friend class weak<T>;
    explicit shared(shared_repr<T> *repr) : data(repr->data), repr(repr) {}

    T *data = nullptr;
    shared_repr<T> *repr = nullptr;
};

/// @ingroup vixen_data_structures
/// @brief Weak reference to a shared pointer's data that does not interfere with the destruction of
/// the shared pointer.
template <typename T>
struct weak {
    using pointer = T *;
    using const_pointer = const T *;

    weak() = default;
    weak(weak<T> &&other);
    weak<T> &operator=(weak<T> &&other);
    ~weak();

    weak<T> copy();
    option<shared<T>> upgrade();

private:
    friend class shared<T>;
    explicit weak(shared_repr<T> *repr) : repr(repr) {}

    shared_repr<T> *repr = nullptr;
};

} // namespace vixen

#include "shared.inl"
