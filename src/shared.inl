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
inline void SharedRepr<T>::acquireStrong() {
    strongCount += 1;
    weakCount += 1;
}

template <typename T>
inline void SharedRepr<T>::releaseStrong() {
    if (strongCount.fetch_sub(1) == 1) {
        heap::destroyInit(alloc, data);
    }
    if (weakCount.fetch_sub(1) == 1) {
        heap::destroyInit(alloc, this);
    }
}

template <typename T>
inline void SharedRepr<T>::acquireWeak() {
    weakCount += 1;
}

template <typename T>
inline void SharedRepr<T>::releaseWeak() {
    if (weakCount.fetch_sub(1) == 1) {
        heap::destroyInit(alloc, this);
    }
}

template <typename T>
inline bool SharedRepr<T>::upgradeWeak() {
    usize count = strongCount.load();
    do {
        if (count == 0) {
            return false;
        }
    } while (!atomic_compare_exchange_weak(&strongCount, &count, count + 1));
    weakCount += 1;
    return true;
}

template <typename T>
template <typename... Args>
inline Shared<T>::Shared(Allocator &alloc, Args &&...args)
    : repr(heap::createInit<SharedRepr<T>>(alloc, alloc, std::forward<Args>(args)...)) {
    data = repr->data;
}

template <typename T>
inline Shared<T>::Shared(Shared<T> &&other)
    : data(std::exchange(other.data, nullptr)), repr(std::exchange(other.repr, nullptr)) {}

template <typename T>
inline Shared<T> &Shared<T>::operator=(Shared<T> &&other) {
    if (std::addressof(other) == this) return *this;

    repr = std::exchange(other.repr, nullptr);
    data = std::exchange(other.data, nullptr);

    return *this;
}

template <typename T>
inline Shared<T>::~Shared() {
    clear();
}

template <typename T>
inline void Shared<T>::clear() {
    if (repr) {
        repr->releaseStrong();
        repr = nullptr;
        data = nullptr;
    }
}

template <typename T>
inline Shared<T> Shared<T>::copy() {
    repr->acquireStrong();
    return Shared{repr};
}

template <typename T>
inline Weak<T> Shared<T>::downgrade() {
    repr->acquireWeak();
    return Weak{repr};
}

template <typename T>
Weak<T>::Weak(Weak<T> &&other) : repr(std::exchange(other.repr, nullptr)) {}

template <typename T>
Weak<T> &Weak<T>::operator=(Weak<T> &&other) {
    if (std::addressof(other) == this) return *this;
    repr = std::exchange(other.repr, nullptr);
    return *this;
}

template <typename T>
inline Weak<T>::~Weak() {
    if (repr) {
        repr->releaseWeak();
    }
}

template <typename T>
inline Weak<T> Weak<T>::copy() {
    repr->acquireWeak();
    return Weak{repr};
}

template <typename T>
inline Option<Shared<T>> Weak<T>::upgrade() {
    if (repr->upgradeWeak()) {
        return Shared{repr};
    } else {
        return {};
    }
}

} // namespace vixen
