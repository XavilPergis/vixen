#include "xen/assert.hpp"

namespace xen {
void default_handler() {}

static assert_handler g_assert_handler = &default_handler;

assert_handler install_assert_handler(assert_handler handler) {
    assert_handler prev = g_assert_handler;
    g_assert_handler = handler;
    return prev;
}

assert_handler remove_assert_handler() {
    return install_assert_handler(&default_handler);
}

namespace detail {
void invoke_assert_handler() {
    g_assert_handler();
}
} // namespace detail

} // namespace xen
