#pragma once

#include "vixen/shared.hpp"

namespace vixen {

template <typename T>
template <typename... Args>
inline shared_repr<T>::shared_repr(allocator *alloc, Args &&...args)
    : alloc(alloc)
    , data(heap::create_init<T>(alloc, std::forward<Args>(args)...))
    , strong_count(1)
    , weak_count(1) {}

template <typename T>
inline void shared_repr<T>::acquire_strong() {
    strong_count += 1;
    weak_count += 1;
}

template <typename T>
inline void shared_repr<T>::release_strong() {
    if (strong_count.fetch_sub(1) == 1) {
        heap::destroy_init(alloc, data);
    }
    if (weak_count.fetch_sub(1) == 1) {
        heap::destroy_init(alloc, this);
    }
}

template <typename T>
inline void shared_repr<T>::acquire_weak() {
    weak_count += 1;
}

template <typename T>
inline void shared_repr<T>::release_weak() {
    if (weak_count.fetch_sub(1) == 1) {
        heap::destroy_init(alloc, this);
    }
}

template <typename T>
inline bool shared_repr<T>::upgrade_weak() {
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
inline shared<T>::shared(allocator *alloc, Args &&...args)
    : repr(heap::create_init<shared_repr<T>>(alloc, alloc, std::forward<Args>(args)...)) {
    data = repr->data;
}

template <typename T>
inline shared<T>::shared(shared<T> &&other)
    : repr(std::exchange(other.repr, nullptr)), data(std::exchange(other.data, nullptr)) {}

template <typename T>
inline shared<T> &shared<T>::operator=(shared<T> &&other) {
    if (std::addressof(other) == this)
        return *this;

    repr = std::exchange(other.repr, nullptr);
    data = std::exchange(other.data, nullptr);

    return *this;
}

template <typename T>
inline shared<T>::~shared() {
    clear();
}

template <typename T>
inline void shared<T>::clear() {
    if (repr) {
        repr->release_strong();
        repr = nullptr;
        data = nullptr;
    }
}

template <typename T>
inline shared<T> shared<T>::copy() {
    repr->acquire_strong();
    return shared{repr};
}

template <typename T>
inline weak<T> shared<T>::downgrade() {
    repr->acquire_weak();
    return weak{repr};
}

template <typename T>
weak<T>::weak(weak<T> &&other) : repr(std::exchange(other.repr, nullptr)) {}

template <typename T>
weak<T> &weak<T>::operator=(weak<T> &&other) {
    if (std::addressof(other) == this)
        return *this;
    repr = std::exchange(other.repr, nullptr);
    return *this;
}

template <typename T>
inline weak<T>::~weak() {
    if (repr) {
        repr->release_weak();
    }
}

template <typename T>
inline weak<T> weak<T>::copy() {
    repr->acquire_weak();
    return weak{repr};
}

template <typename T>
inline option<shared<T>> weak<T>::upgrade() {
    if (repr->upgrade_weak()) {
        return shared{repr};
    } else {
        return {};
    }
}

} // namespace vixen
