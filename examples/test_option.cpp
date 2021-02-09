#include <vixen/option.hpp>

using namespace vixen;

struct noisy {
    // clang-format off
    noisy() { VIXEN_INFO("noisy defaulted!"); }
    noisy(const noisy &) { VIXEN_INFO("noisy copy constructed!"); }
    noisy(noisy &&other) { other.foo = 4321; VIXEN_INFO("noisy move constructed!"); }
    noisy &operator=(const noisy &) { VIXEN_INFO("noisy copy assigned!"); return *this; }
    noisy &operator=(noisy &&other) { other.foo = 4321; VIXEN_INFO("noisy move assigned!"); return *this; }
    ~noisy() { if (foo == 1234) { VIXEN_INFO("noisy destructed!"); } else { VIXEN_INFO("moved-from noisy destructed!"); } }
    // clang-format on

    int foo = 1234;
};

int main() {
    {
        VIXEN_WARN("Default initialization");
        option<noisy> foo;
        VIXEN_ASSERT(foo.is_none());
        // NOTE: should not print anything here
    }

    {
        VIXEN_WARN("Value move constructor");
        option<noisy> foo = noisy();
        VIXEN_ASSERT(foo.is_some() && foo->foo == 1234);
    }

    {
        VIXEN_WARN("Value copy constructor");
        noisy bar;
        option<noisy> foo = bar;
        VIXEN_ASSERT(foo.is_some() && foo->foo == bar.foo);
    }

    {
        VIXEN_WARN("Value move assignment operator");
        option<noisy> foo;
        foo = noisy();
        VIXEN_ASSERT(foo.is_some() && foo->foo == 1234);
    }

    {
        VIXEN_WARN("Value copy assignment operator");
        noisy bar;
        option<noisy> foo;
        foo = bar;
        VIXEN_ASSERT(foo.is_some() && foo->foo == bar.foo);
    }
}
