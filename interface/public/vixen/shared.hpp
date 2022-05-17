#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/option.hpp"
#include "vixen/types.hpp"

#include <atomic>

namespace vixen {

// SAFETY: this struct should only be used behind a pointer that was allocated via the allocator
// referred to by `alloc`.
template <typename T>
struct SharedRepr {
    std::atomic<usize> strongCount;
    std::atomic<usize> weakCount;

    Allocator *alloc;
    T *data;

    template <typename... Args>
    SharedRepr(Allocator &alloc, Args &&...args);

    void acquireStrong();
    void releaseStrong();
    void acquireWeak();
    void releaseWeak();

    bool upgradeWeak();
};

template <typename T>
struct Weak;

/// @ingroup vixen_data_structures
/// @brief Managed pointer that destroys its allocation when all other shared pointers to the same
/// object are destroyed.
template <typename T>
struct Shared {
    using pointer = T *;
    using const_pointer = const T *;

    template <typename... Args>
    Shared(Allocator &alloc, Args &&...args);

    Shared() = default;
    Shared(Shared<T> &&other);
    Shared<T> &operator=(Shared<T> &&other);
    ~Shared();

    // clang-format off
    const T &operator*() const { return *data; }
    const T *operator->() const { return data; }
    T &operator*() { return *data; }
    T *operator->() { return data; }

    const T &get() const { return *data; }
    T &get() { return *data; }

    explicit operator bool() const { return data; }
    operator const_pointer() const { return data; }
    operator pointer() { return data; }
    // clang-format on

    void clear();
    Shared<T> copy();
    Weak<T> downgrade();

    template <typename S>
    S &operator<<(S &s) const {
        s << **this;
        return s;
    }

private:
    friend class Weak<T>;
    explicit Shared(SharedRepr<T> *repr) : data(repr->data), repr(repr) {}

    T *data = nullptr;
    SharedRepr<T> *repr = nullptr;
};

/// @ingroup vixen_data_structures
/// @brief Weak reference to a shared pointer's data that does not interfere with the destruction of
/// the shared pointer.
template <typename T>
struct Weak {
    using pointer = T *;
    using const_pointer = const T *;

    Weak() = default;
    Weak(Weak<T> &&other);
    Weak<T> &operator=(Weak<T> &&other);
    ~Weak();

    Weak<T> copy();
    Option<Shared<T>> upgrade();

private:
    friend class Shared<T>;
    explicit Weak(SharedRepr<T> *repr) : repr(repr) {}

    SharedRepr<T> *repr = nullptr;
};

template <typename T, typename... Args>
Shared<T> makeShared(Allocator &alloc, Args &&...args) {
    return Shared<T>(alloc, std::forward<Args>(args)...);
}

} // namespace vixen

#include "shared.inl"
