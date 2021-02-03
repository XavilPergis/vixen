#pragma once

#include "vixen/assert.hpp"
#include "vixen/traits.hpp"
#include "vixen/types.hpp"
#include "vixen/util.hpp"

#include "erased_storage.hpp"

namespace vixen {

namespace detail {
template <typename T>
struct ok {
    T value;
};
template <typename T>
struct err {
    T value;
};
} // namespace detail

template <typename T, typename E>
struct result_impl {
    template <typename U>
    result_impl(detail::ok<U> &&val) : ok_flag(true) {
        ok_storage.set(std::forward<U>(val.value));
    }
    template <typename U>
    result_impl(detail::err<U> &&val) : ok_flag(false) {
        err_storage.set(std::forward<U>(val.value));
    }

    result_impl() = delete;

    ~result_impl() {
        erase();
    }

    result_impl(result_impl<T, E> &&other) {
        if (other.is_ok()) {
            ok_storage.set(mv(other.get_ok()));
        } else {
            err_storage.set(mv(other.get_err()));
        }
    }

    result_impl<T, E> &operator=(result_impl<T, E> &&other) {
        if (std::addressof(other) == this)
            return *this;

        erase();

        if (other.is_ok()) {
            ok_storage.set(mv(other.get_ok()));
        } else {
            err_storage.set(mv(other.get_err()));
        }

        return *this;
    }

    // clang-format off
    bool is_ok() const { return ok_flag; }

    template <typename U>
    void set_ok(U &&val) { return ok_storage.set(std::forward<U>(val)); }
    T &get_ok() { return ok_storage.get(); }
    const T &get_ok() const { return ok_storage.get(); }

    template <typename U>
    void set_err(U &&val) { return err_storage.set(std::forward<U>(val)); }
    T &get_err() { return err_storage.get(); }
    const T &get_err() const { return err_storage.get(); }
    // clang-format on

    void erase() {
        if (ok_flag) {
            ok_storage.erase();
        } else {
            err_storage.erase();
        }
    }

    bool ok_flag;
    union {
        uninitialized_storage<T> ok_storage;
        uninitialized_storage<E> err_storage;
    };
};

template <typename T, typename E>
struct result {
    template <typename U>
    result(detail::ok<U> &&val) : impl(mv(val)) {}
    template <typename U>
    result(detail::err<U> &&val) : impl(mv(val)) {}

    result(result<T, E> &&other) = default;
    result<T, E> &operator=(result<T, E> &&other) = default;

    bool operator==(const result<T, E> &rhs) const {
        if (impl.is_ok()) {
            return rhs.impl.is_ok() && impl.get_ok() == rhs.impl.get_ok();
        } else {
            return rhs.impl.is_err() && impl.get_err() == rhs.impl.get_err();
        }
    }

    bool operator!=(const result<T, E> &rhs) const {
        if (impl.is_ok()) {
            return rhs.impl.is_err() || impl.get_ok() != rhs.impl.get_ok();
        } else {
            return rhs.impl.is_ok() || impl.get_err() != rhs.impl.get_err();
        }
    }

    bool is_ok() const {
        return impl.is_ok();
    }
    bool is_err() const {
        return !impl.is_ok();
    }

    T &unwrap_ok() {
        VIXEN_DEBUG_ASSERT(impl.is_ok(), "tried to unwrap result with err value.");
        return impl.get_ok();
    }
    T &unwrap_err() {
        VIXEN_DEBUG_ASSERT(!impl.is_ok(), "tried to err-unwrap result with ok value.");
        return impl.get_err();
    }
    const T &unwrap_ok() const {
        VIXEN_DEBUG_ASSERT(impl.is_ok(), "tried to unwrap result with err value.");
        return impl.get_ok();
    }
    const T &unwrap_err() const {
        VIXEN_DEBUG_ASSERT(!impl.is_ok(), "tried to err-unwrap result with ok value.");
        return impl.get_err();
    }

    option<T> to_ok() {
        if (impl.is_ok()) {
            return mv(impl.get_ok());
        } else {
            return nullptr;
        }
    }
    option<T> to_err() {
        if (!impl.is_ok()) {
            return mv(impl.get_err());
        } else {
            return nullptr;
        }
    }

    option<T &> ok() {
        if (impl.is_ok()) {
            return impl.get_ok();
        } else {
            return nullptr;
        }
    }
    option<T &> err() {
        if (!impl.is_ok()) {
            return impl.get_err();
        } else {
            return nullptr;
        }
    }
    option<const T &> ok() const {
        if (impl.is_ok()) {
            return impl.get_ok();
        } else {
            return nullptr;
        }
    }
    option<const T &> err() const {
        if (!impl.is_ok()) {
            return impl.get_err();
        } else {
            return nullptr;
        }
    }

private:
    result_impl<T, E> impl;
};

template <typename T>
inline detail::ok<std::remove_reference_t<T>> ok(T &&val) {
    return detail::ok<std::remove_reference_t<T>>{std::forward<T>(val)};
}

template <typename E>
inline detail::err<std::remove_reference_t<E>> err(E &&val) {
    return detail::err<std::remove_reference_t<E>>{std::forward<E>(val)};
}

template <typename T>
inline T unify_result(result<T, T> &&val) {
    return val.is_ok() ? mv(val.unwrap_ok()) : mv(val.unwrap_err());
}

} // namespace vixen
