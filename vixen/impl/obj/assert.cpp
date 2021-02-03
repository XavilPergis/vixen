#include "vixen/assert.hpp"

#include "vixen/allocator/allocator.hpp"
#include "vixen/allocator/stacktrace.hpp"

#include <dlfcn.h>

namespace vixen {

logger_id panic_logger = create_logger("vixen-panic", [](logger_id id) {
    set_logger_format_string(id,
        "\n%^panic:%$ in `%!` at %g:%# ~ %P/%t ~ %n/%l\n\n\x1b[31;1m# Panic Message\x1b[0m\n%v\n");
});

logger_id panic_stacktrace_logger = create_logger("vixen-panic-stacktrace", [](logger_id id) {
    set_logger_format_string(id, "%^=>%$ %v");
});

usize get_base_offset(void *trace_addr) {
    Dl_info info;
    dladdr(trace_addr, &info);

    return (usize)trace_addr - (usize)info.dli_fbase;
}

void default_handler() {
    translation_cache cache(heap::debug_allocator());
    vector<void *> raw_trace = capture_stack_trace(heap::debug_allocator());

    void *barrier_base = (void *)detail::invoke_panic_handler;

    vector<address_info> translated_barrier = translate_stack_trace(&cache, {&barrier_base, 1});
    vector<address_info> trace = translate_stack_trace(&cache, raw_trace);

    usize barrier_idx = 0;
    for (usize i = 0; i < trace.len(); ++i) {
        if (trace[i].name == translated_barrier[0].name) {
            barrier_idx = i;
            break;
        }
    }

    VIXEN_ERROR_EXT(panic_stacktrace_logger,
        "\x1b[1mA critical error has been encountered, and program execution could not continue.\x1b[0m")
    VIXEN_ERROR_EXT(panic_stacktrace_logger,
        "\x1b[1mBelow is a trace of the current execution path.\x1b[0m\n")

    if (barrier_idx > 0) {
        VIXEN_INFO_EXT(panic_stacktrace_logger, "...")
    }

    for (usize i = 0; i < trace.len(); ++i) {
        if (barrier_idx < i) {
            VIXEN_INFO_EXT(panic_stacktrace_logger,
                "\x1b[34;1m{: >4}\x1b[0m :: \x1b[33m{}\x1b[0m",
                i + 1,
                trace[i].name)
            VIXEN_INFO_EXT(panic_stacktrace_logger,
                "      \x1b[30;1min \x1b[0m\x1b[30m{}\x1b[0m",
                trace[i].location)
            VIXEN_INFO_EXT(panic_stacktrace_logger,
                "      \x1b[30;1msymbol \x1b[0m\x1b[30m{}\x1b[0m",
                trace[i].raw_symbol)
            VIXEN_INFO_EXT(panic_stacktrace_logger, "")
        }
    }
}

static panic_handler g_panic_handler = &default_handler;
static bool g_already_panicking = false;

panic_handler install_panic_handler(panic_handler handler) {
    panic_handler prev = g_panic_handler;
    g_panic_handler = handler;
    return prev;
}

panic_handler remove_panic_handler() {
    return install_panic_handler(&default_handler);
}

namespace detail {
void invoke_panic_handler() {
    if (!g_already_panicking)
        g_panic_handler();
    ::std::abort();
}
} // namespace detail

} // namespace vixen
