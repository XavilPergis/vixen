#include "vixen/allocator/stacktrace.hpp"
#include "vixen/assert.hpp"

#if defined(VIXEN_PLATFORM_WINDOWS)
#include <windows.h>
// windows.h has to come before dbghelp.h for some reason with my mingw version?
#include <dbghelp.h>
#include <winnt.h>
#endif

namespace vixen::platform {

StackTraceCapabilities getStackTraceCapabilities() {
    StackTraceCapabilities caps{};

#if defined(VIXEN_PLATFORM_WINDOWS)
    caps.supportsStackTraceCapture = true;
    // not yet at least
    caps.supportsStackTraceResolution = false;
#endif

    return caps;
}

#if defined(VIXEN_PLATFORM_WINDOWS)
void captureStackFrames(StackTraceCaptureContext &opts) {
    VIXEN_DEBUG_ASSERT(opts.framesToCapture >= opts.framesToSkip);

    auto toSkip = static_cast<DWORD>(opts.framesToCapture - opts.framesToSkip);
    auto toCapture = static_cast<DWORD>(opts.framesToCapture);
    auto captured = RtlCaptureStackBackTrace(toSkip, toCapture, opts.outFrames, nullptr);

    if (opts.outFramesCount)
        *opts.outFramesCount = static_cast<usize>(captured);
}
#endif

} // namespace vixen::platform
