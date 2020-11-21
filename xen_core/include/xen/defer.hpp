#pragma once

#include <exception>

#define _DEFER_1(x, y) x##y
#define _DEFER_2(x, y) _DEFER_1(x, y)
#define _DEFER_3(x) _DEFER_2(x, __COUNTER__)

// The empty block beforehand prevents users assigning the defer to a usable varible.
#define defer(code)                                          \
    {};                                                      \
    auto _DEFER_3(_defer_) = ::xen::detail::make_defer([&] { \
        code;                                                \
    })
#define errdefer(code)                                          \
    {};                                                         \
    auto _DEFER_3(_defer_) = ::xen::detail::make_errdefer([&] { \
        code;                                                   \
    })

namespace xen {
namespace detail {
template <typename F>
struct Defer {
    Defer(F func) : func(func), is_active(true) {}
    ~Defer() {
        if (is_active) {
            this->func();
        }
    }

    Defer(const Defer &) = delete;
    Defer(const Defer &&) = delete;
    void operator=(const Defer &) = delete;

    Defer(Defer &&other) : func(other.func), is_active(true) {
        other.is_active = false;
    }

private:
    bool is_active;
    F func;
};

template <typename F>
struct ErrDefer {
    ErrDefer(F func) : func(func), exception_count(std::uncaught_exceptions()), is_active(true) {}
    ~ErrDefer() {
        if (is_active && std::uncaught_exceptions() > exception_count) {
            this->func();
        }
    }

    ErrDefer(const ErrDefer &) = delete;
    ErrDefer(const ErrDefer &&) = delete;
    void operator=(const ErrDefer &) = delete;

    ErrDefer(ErrDefer &&other) : func(other.func), is_active(true) {
        other.is_active = false;
    }

private:
    bool is_active;
    F func;
    int exception_count;
};

template <typename F>
Defer<F> make_defer(F func) {
    return Defer<F>(func);
}

template <typename F>
ErrDefer<F> make_errdefer(F func) {
    return ErrDefer<F>(func);
}
} // namespace detail

} // namespace xen
