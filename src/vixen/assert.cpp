#include "vixen/assert.hpp"

#include "vixen/allocator/allocator.hpp"
#include "vixen/allocator/stacktrace.hpp"
// #include "vixen/stream.hpp"

namespace vixen {

LoggerId panic_logger = create_logger("vixen-panic", [](LoggerId id) {
    set_logger_format_string(id,
        "\n%^panic:%$ in `%!` at %g:%# ~ %P/%t ~ %n/%l\n\n\x1b[31;1m# Panic Message\x1b[0m\n%v\n");
});

LoggerId panic_stacktrace_logger = create_logger("vixen-panic-stacktrace", [](LoggerId id) {
    set_logger_format_string(id, "%^=>%$ %v");
});

void defaultPanicHandler() {
    auto trace = StackTrace::captureCurrent(heap::debugAllocator());

    VIXEN_ERROR_EXT(panic_stacktrace_logger,
        "\x1b[1mA critical error has been encountered, and program execution could not continue.\x1b[0m")
    VIXEN_ERROR_EXT(panic_stacktrace_logger,
        "\x1b[1mBelow is a trace of the current execution path.\x1b[0m\n")

    Vector<char> charData(heap::debugAllocator());
    auto oi = charData.insertLastIterator();
    formatStackTrace(oi, trace);

    String stackTrace(mv(charData));
    VIXEN_INFO_EXT(panic_stacktrace_logger, "{}", stackTrace);
}

static PanicHandler gPanicHandler = &defaultPanicHandler;
static bool gAlreadyPanicking = false;

PanicHandler installPanicHandler(PanicHandler handler) {
    PanicHandler prev = gPanicHandler;
    gPanicHandler = handler;
    return prev;
}

PanicHandler removePanicHandler() {
    return installPanicHandler(&defaultPanicHandler);
}

namespace detail {
[[noreturn]] void invokePanicHandler() {
    if (!gAlreadyPanicking) {
        gAlreadyPanicking = true;
        gPanicHandler();
        ::std::abort();
    } else {
        VIXEN_CRITICAL("panicked while panicking! aborting.");
        ::std::abort();
    }
}
} // namespace detail

} // namespace vixen
