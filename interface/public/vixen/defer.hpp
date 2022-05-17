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
struct DeferImpl {
    DeferImpl(F func) : func(func), isActive(true) {}
    ~DeferImpl() {
        if (isActive) this->func();
    }

    DeferImpl(const DeferImpl &) = delete;
    void operator=(const DeferImpl &) = delete;

    DeferImpl(DeferImpl &&other) : func(other.func), isActive(other.isActive) {
        other.isActive = false;
    }

private:
    F func;
    bool isActive;
};

template <typename F>
struct ErrDeferImpl {
    ErrDeferImpl(F func) : func(func), exceptionCount(std::uncaught_exceptions()), isActive(true) {}
    ~ErrDeferImpl() {
        if (isActive && std::uncaught_exceptions() > exceptionCount) {
            this->func();
        }
    }

    ErrDeferImpl(const ErrDeferImpl &) = delete;
    void operator=(const ErrDeferImpl &) = delete;

    ErrDeferImpl(ErrDeferImpl &&other)
        : func(other.func), exceptionCount(other.exceptionCount), isActive(other.isActive) {
        other.isActive = false;
        other.exceptionCount = 0;
    }

private:
    F func;
    int exceptionCount;
    bool isActive;
};

template <typename F>
DeferImpl<F> make_defer(F func) {
    return DeferImpl<F>(func);
}

template <typename F>
ErrDeferImpl<F> make_errdefer(F func) {
    return ErrDeferImpl<F>(func);
}
} // namespace detail

} // namespace vixen
