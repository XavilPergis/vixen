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
        VIXEN_WARN("default constructor");
        Option<noisy> opt;
        VIXEN_ASSERT(opt.isNone());
        // NOTE: should not print anything here
    }

    {
        VIXEN_WARN("move constructor");
        Option<noisy> opt = noisy();
        VIXEN_ASSERT(opt.isSome() && opt->opt == 1234);
    }

    {
        VIXEN_WARN("copy constructor");
        noisy bar;
        Option<noisy> opt = bar;
        VIXEN_ASSERT(opt.isSome() && opt->opt == bar.opt);
    }

    {
        VIXEN_WARN("move assignment operator");
        Option<noisy> opt;
        opt = noisy();
        VIXEN_ASSERT(opt.isSome() && opt->opt == 1234);
    }

    {
        VIXEN_WARN("copy assignment operator");
        noisy bar;
        Option<noisy> opt;
        opt = bar;
        VIXEN_ASSERT(opt.isSome() && opt->opt == bar.opt);
    }
}
