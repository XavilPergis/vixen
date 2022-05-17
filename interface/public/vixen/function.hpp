#pragma once

#include "vixen/unique.hpp"

namespace vixen {

template <typename F>
struct function_traits : function_traits<decltype(&F::operator())> {};

template <typename R, typename... Args>
struct function_traits<R(Args...)> {
    using return_type = R;
    static constexpr usize arity = sizeof...(Args);

private:
    template <usize I>
    struct get_argument {
        static_assert(I < arity, "invalid parameter index");
        using type = select<I, unpack<Args...>>;
    };

public:
    template <usize I>
    using argument = typename get_argument<I>::type;
};

template <typename R, typename... Args>
struct function_traits<R (*)(Args...)> : public function_traits<R(Args...)> {};

template <typename R, typename... Args>
struct function_traits<R (&)(Args...)> : public function_traits<R(Args...)> {};

template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)> : public function_traits<R(Args...)> {};

template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const> : public function_traits<R(Args...)> {};

struct Runnable {
    virtual ~Runnable() {}
    virtual void operator()() = 0;
};

template <typename F>
struct TypedRunnable : Runnable {
    TypedRunnable(F &&functor) : functor(std::forward<F>(functor)) {}

    virtual void operator()() override { functor(); }

    F functor;
};

template <typename F>
Unique<Runnable> makeRunnable(Allocator &alloc, F &&func) {
    return makeUnique<TypedRunnable<F>>(alloc, std::forward<F>(func));
}

} // namespace vixen
