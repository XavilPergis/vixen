#pragma once

#include <exception>

#define _DEFER_1(x, y) x##y
#define _DEFER_2(x, y) _DEFER_1(x, y)
#define _DEFER_3(x) _DEFER_2(x, __COUNTER__)

// The empty block beforehand prevents users assigning the defer to a usable varible.
#define defer(code)                                            \
    {};                                                        \
    auto _DEFER_3(_defer_) = ::vixen::detail::make_defer([&] { \
        code;                                                  \
    })
#define errdefer(code)                                            \
    {};                                                           \
    auto _DEFER_3(_defer_) = ::vixen::detail::make_errdefer([&] { \
        code;                                                     \
    })

namespace vixen {
namespace detail {
template <typename F>
struct defer_ty {
    defer_ty(F func) : func(func), is_active(true) {}
    ~defer_ty() {
        if (is_active) {
            this->func();
        }
    }

    defer_ty(const defer_ty &) = delete;
    defer_ty(const defer_ty &&) = delete;
    void operator=(const defer_ty &) = delete;

    defer_ty(defer_ty &&other) : func(other.func), is_active(true) {
        other.is_active = false;
    }

private:
    F func;
    bool is_active;
};

template <typename F>
struct err_defer_ty {
    err_defer_ty(F func)
        : func(func), exception_count(std::uncaught_exceptions()), is_active(true) {}
    ~err_defer_ty() {
        if (is_active && std::uncaught_exceptions() > exception_count) {
            this->func();
        }
    }

    err_defer_ty(const err_defer_ty &) = delete;
    err_defer_ty(const err_defer_ty &&) = delete;
    void operator=(const err_defer_ty &) = delete;

    err_defer_ty(err_defer_ty &&other) : func(other.func), is_active(true) {
        other.is_active = false;
    }

private:
    F func;
    int exception_count;
    bool is_active;
};

template <typename F>
defer_ty<F> make_defer(F func) {
    return defer_ty<F>(func);
}

template <typename F>
err_defer_ty<F> make_errdefer(F func) {
    return err_defer_ty<F>(func);
}
} // namespace detail

} // namespace vixen
