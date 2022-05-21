#pragma once

#include "vixen/shared.hpp"

namespace vixen {

template <typename T>
template <typename... Args>
inline SharedRepr<T>::SharedRepr(Allocator &alloc, Args &&...args)
    : strongCount(1)
    , weakCount(1)
    , alloc(&alloc)
    , data(heap::createInit<T>(alloc, std::forward<Args>(args)...)) {}

template <typename T>
inline void SharedRepr<T>::acquireStrong() noexcept {
    // THIS LOOKS SO WRONG, but i think its fine! because we'll always be holding onto another
    // Shared when this is called, its impossible for the resource to be destroyed by
    // releaseStrong(). If it's the only other shared that's referencing this resource, then
    // strongCount will be 1 before this operation, but that will be okay since releaseStrong()
    // can't be reached until this function completes. If there are more than one, then
    // releaseStrong can never observe a count of 0.
    strongCount += 1;

    // we pretend that every strong reference has a "phantom" weak reference attached to it
    weakCount += 1;
}

template <typename T>
inline void SharedRepr<T>::releaseStrong() noexcept {
    // see acquireStrong() for why this is safe
    if (strongCount.fetch_sub(1) == 1) heap::destroyInit(*alloc, data);
    if (weakCount.fetch_sub(1) == 1) heap::destroyInit(*alloc, this);
}

template <typename T>
inline void SharedRepr<T>::acquireWeak() noexcept {
    // see acquireStrong() for why this wacky ass code is okay
    weakCount += 1;
}

template <typename T>
inline void SharedRepr<T>::releaseWeak() noexcept {
    // see acquireStrong() for why this is safe
    if (weakCount.fetch_sub(1) == 1) {
        heap::destroyInit(*alloc, this);
    }
}

template <typename T>
inline UpgradeResult SharedRepr<T>::upgradeWeak() noexcept {
    // ok, we ACTUALLY havew to be careful here, and use a simple CAS loop to update the strong
    // count. We can't just increment it because a thread may observe it to be 1 and destroy the
    // resource just before we update it to be 2 and construct a new strong reference.
    usize count = strongCount.load();
    do {
        // fail if the count is ever 0, because that means the resource was successfully destroyed.
        if (count == 0) return UpgradeResult::FAILED;
    } while (!strongCount.compare_exchange_weak(count, count + 1));

    // see acquireStrong() for why this is safe
    weakCount += 1;
    return UpgradeResult::UPGRADED;
}

template <typename T>
template <typename... Args>
inline Shared<T>::Shared(Allocator &alloc, Args &&...args)
    : mRepr(heap::createInit<SharedRepr<T>>(alloc, alloc, std::forward<Args>(args)...)) {
    mData = mRepr->data;
}

template <typename T>
inline Shared<T>::Shared(Shared<T> &&other) noexcept
    : mData(util::exchange(other.mData, nullptr)), mRepr(util::exchange(other.mRepr, nullptr)) {}

template <typename T>
inline Shared<T> &Shared<T>::operator=(Shared<T> &&other) noexcept {
    if (std::addressof(other) == this) return *this;

    mRepr = util::exchange(other.mRepr, nullptr);
    mData = util::exchange(other.mData, nullptr);

    return *this;
}

template <typename T>
inline Shared<T>::~Shared() {
    release();
}

template <typename T>
inline void Shared<T>::release() noexcept {
    if (!mRepr) return;
    mRepr->releaseStrong();
    mRepr = nullptr;
    mData = nullptr;
}

template <typename T>
inline Shared<T> Shared<T>::copy() noexcept {
    mRepr->acquireStrong();
    return Shared{mRepr};
}

template <typename T>
inline Weak<T> Shared<T>::downgrade() noexcept {
    mRepr->acquireWeak();
    return Weak{mRepr};
}

template <typename T>
Weak<T>::Weak(Weak<T> &&other) noexcept : mRepr(util::exchange(other.mRepr, nullptr)) {}

template <typename T>
Weak<T> &Weak<T>::operator=(Weak<T> &&other) noexcept {
    if (&other == this) return *this;
    mRepr = util::exchange(other.mRepr, nullptr);
    return *this;
}

template <typename T>
inline Weak<T>::~Weak() {
    if (mRepr) mRepr->releaseWeak();
}

template <typename T>
inline Weak<T> Weak<T>::copy() noexcept {
    mRepr->acquireWeak();
    return Weak{mRepr};
}

template <typename T>
inline Shared<T> Weak<T>::upgrade() noexcept {
    if (mRepr->upgradeWeak() == UpgradeResult::FAILED) return {};
    return Shared{mRepr};
}

} // namespace vixen
