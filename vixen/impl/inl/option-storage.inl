#pragma once

#include "vixen/option.hpp"

#include "erased_storage.hpp"

namespace vixen {

// Defines how the option is encoded and how the encoding is accessed. The weird inheritance is so
// that when this class is specialized, only the four accessor methods need to be redefined, instead
// of the whole arsenal of methods on `Option`.
template <typename T>
struct option_impl {
    option_impl() : occupied(false) {}

    ~option_impl() {
        clear();
    }

    option_impl(option_impl &&other)
        : occupied(std::exchange(other.occupied, false)), storage(other.storage) {}
    option_impl &operator=(option_impl &&other) {
        if (std::addressof(other) == this)
            return *this;
        occupied = std::exchange(other.occupied, false);
        storage = other.storage;
        return *this;
    }

    // clang-format off
    T &get() { return storage.get(); }
    const T &get() const { return storage.get(); }
    bool get_occupied() const { return occupied; }
    // clang-format on

    template <typename U>
    void set(U &&value) {
        if (occupied)
            storage.erase();
        storage.set(std::forward<U>(value));
        occupied = true;
    }

    void clear() {
        if (occupied)
            storage.erase();
        occupied = false;
    }

private:
    uninitialized_storage<T> storage;
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
