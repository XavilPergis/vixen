#pragma once

#include "vixen/shared.hpp"

namespace vixen {

template <typename T>
template <typename... Args>
inline SharedRepr<T>::SharedRepr(Allocator *alloc, Args &&...args)
    : strong_count(1)
    , weak_count(1)
    , alloc(alloc)
    , data(heap::create_init<T>(alloc, std::forward<Args>(args)...)) {}

template <typename T>
inline void SharedRepr<T>::acquire_strong() {
    strong_count += 1;
    weak_count += 1;
}

template <typename T>
inline void SharedRepr<T>::release_strong() {
    if (strong_count.fetch_sub(1) == 1) {
        heap::destroy_init(alloc, data);
    }
    if (weak_count.fetch_sub(1) == 1) {
        heap::destroy_init(alloc, this);
    }
}

template <typename T>
inline void SharedRepr<T>::acquire_weak() {
    weak_count += 1;
}

template <typename T>
inline void SharedRepr<T>::release_weak() {
    if (weak_count.fetch_sub(1) == 1) {
        heap::destroy_init(alloc, this);
    }
}

template <typename T>
inline bool SharedRepr<T>::upgrade_weak() {
    usize count = strong_count.load();
    do {
        if (count == 0) {
            return false;
        }
    } while (!atomic_compare_exchange_weak(&strong_count, &count, count + 1));
    weak_count += 1;
    return true;
}

template <typename T>
template <typename... Args>
inline Shared<T>::Shared(Allocator *alloc, Args &&...args)
    : repr(heap::create_init<SharedRepr<T>>(alloc, alloc, std::forward<Args>(args)...)) {
    data = repr->data;
}

template <typename T>
inline Shared<T>::Shared(Shared<T> &&other)
    : data(std::exchange(other.data, nullptr)), repr(std::exchange(other.repr, nullptr)) {}

template <typename T>
inline Shared<T> &Shared<T>::operator=(Shared<T> &&other) {
    if (std::addressof(other) == this)
        return *this;

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
        repr->release_strong();
        repr = nullptr;
        data = nullptr;
    }
}

template <typename T>
inline Shared<T> Shared<T>::copy() {
    repr->acquire_strong();
    return Shared{repr};
}

template <typename T>
inline Weak<T> Shared<T>::downgrade() {
    repr->acquire_weak();
    return Weak{repr};
}

template <typename T>
Weak<T>::Weak(Weak<T> &&other) : repr(std::exchange(other.repr, nullptr)) {}

template <typename T>
Weak<T> &Weak<T>::operator=(Weak<T> &&other) {
    if (std::addressof(other) == this)
        return *this;
    repr = std::exchange(other.repr, nullptr);
    return *this;
}

template <typename T>
inline Weak<T>::~Weak() {
    if (repr) {
        repr->release_weak();
    }
}

template <typename T>
inline Weak<T> Weak<T>::copy() {
    repr->acquire_weak();
    return Weak{repr};
}

template <typename T>
inline Option<Shared<T>> Weak<T>::upgrade() {
    if (repr->upgrade_weak()) {
        return Shared{repr};
    } else {
        return {};
    }
}

} // namespace vixen
