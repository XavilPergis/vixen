#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/option.hpp"
#include "vixen/types.hpp"

#include <atomic>

namespace vixen {

enum class UpgradeResult {
    UPGRADED,
    FAILED,
};

// SAFETY: this struct should only be used behind a pointer that was allocated via the allocator
// referred to by `alloc`.
template <typename T>
struct SharedRepr {
    template <typename... Args>
    SharedRepr(Allocator &alloc, Args &&...args);

    void acquireStrong();
    void releaseStrong();
    void acquireWeak();
    void releaseWeak();
    UpgradeResult upgradeWeak();

    std::atomic<usize> strongCount;
    std::atomic<usize> weakCount;

    Allocator *alloc;
    T *data;
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
    Shared(Shared<T> &&other) noexcept;
    Shared<T> &operator=(Shared<T> &&other) noexcept;
    ~Shared();

    // clang-format off
    const T &operator*() const noexcept { return *mData; }
    const T *operator->() const noexcept { return mData; }
    T &operator*() noexcept { return *mData; }
    T *operator->() noexcept { return mData; }

    const T &get() const { return *mData; }
    T &get() { return *mData; }

    explicit operator bool() const { return mData; }
    operator const_pointer() const { return mData; }
    operator pointer() { return mData; }
    // clang-format on

    void release();
    Shared<T> copy();
    Weak<T> downgrade();

    template <typename S>
    S &operator<<(S &s) const {
        s << **this;
        return s;
    }

private:
    friend class Weak<T>;
    explicit Shared(SharedRepr<T> *repr) noexcept : mData(repr->data), mRepr(repr) {}

    T *mData = nullptr;
    SharedRepr<T> *mRepr = nullptr;
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
    explicit Weak(SharedRepr<T> *repr) noexcept : mRepr(repr) {}

    SharedRepr<T> *mRepr = nullptr;
};

template <typename T, typename... Args>
Shared<T> makeShared(Allocator &alloc, Args &&...args) {
    return Shared<T>(alloc, std::forward<Args>(args)...);
}

} // namespace vixen

#include "shared.inl"
