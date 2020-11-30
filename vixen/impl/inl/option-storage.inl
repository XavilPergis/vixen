#pragma once

#include "vixen/option.hpp"

namespace vixen {

template <typename T>
struct option_storage {
    template <typename U>
    void set(U &&new_value) {
        util::construct_in_place(reinterpret_cast<T *>(value), std::forward<U>(new_value));
    }

    T &get() {
        return *reinterpret_cast<T *>(value);
    }

    T const &get() const {
        return *reinterpret_cast<const T *>(value);
    }

    void deinit() {
        get().~T();
    }

    // byte array with the same size and alignment as T so default construction of `option_storage`
    // doesn't default initialize `value`.
    alignas(T) u8 value[sizeof(T)];
};

// Defines how the option is encoded and how the encoding is accessed. The weird inheritance is so
// that when this class is specialized, only the four accessor methods need to be redefined, instead
// of the whole arsenal of methods on `Option`.
template <typename T>
struct option_impl {
    option_impl() : occupied(false) {}

    ~option_impl() {
        clear();
    }

    const T &get() const {
        return storage.get();
    }

    T &get() {
        return storage.get();
    }

    bool get_occupied() const {
        return occupied;
    }

    template <typename U>
    void set(U &&value) {
        if (occupied)
            storage.deinit();
        storage.set(std::forward<U>(value));
        occupied = true;
    }

    void clear() {
        if (occupied)
            storage.deinit();
        occupied = false;
    }

private:
    option_storage<T> storage;
    bool occupied;
};

// Specialization for `T&` so that it takes up as much space as `T*`. Since `T&` is never null, we
// can encode none as that value.
template <typename T>
struct option_impl<T &> {
    option_impl() : value(nullptr) {}
    option_impl(T &value) : value(&value) {}

    const T &get() const {
        return *value;
    }

    T &get() {
        return *value;
    }

    bool get_occupied() const {
        return value != nullptr;
    }

    void set(T &value) {
        this->value = std::addressof(value);
    }

    void clear() {
        value = nullptr;
    }

private:
    T *value;
};

// See specialization for `T&`
template <typename T>
struct option_impl<const T &> {
    option_impl() : value(nullptr) {}
    option_impl(T const &value) : value(&value) {}

    const T &get() const {
        return *value;
    }

    bool get_occupied() const {
        return value != nullptr;
    }

    void set(T const &value) {
        this->value = std::addressof(value);
    }

    void clear() {
        value = nullptr;
    }

private:
    const T *value;
};

} // namespace vixen
