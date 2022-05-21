#include "vixen/allocator/stacktrace.hpp"

#include "vixen/assert.hpp"
#include "vixen/defer.hpp"
#include "vixen/util.hpp"

namespace vixen {

StackTrace StackTrace::captureCurrent(Allocators alloc, CaptureOptions captureOpts) {
    auto caps = platform::getStackTraceCapabilities();
    VIXEN_ASSERT(caps.supportsStackTraceCapture);

    StackTrace trace(alloc.persistent());

    platform::StackTraceAddrsCaptureContext opts{};
    opts.allocators = alloc;
    opts.skipSelfFrames = captureOpts.skipSelfFrames;
    opts.framesToSkip = captureOpts.skipFrames + (captureOpts.skipSelfFrames ? 1 : 0);
    opts.maxFramesToCapture = captureOpts.captureFrames;

    platform::captureStackAddrs(trace, opts);
    trace.callStackFrames.reverse();

    return trace;
}

bool StackTrace::resolveSourceLocations() {
    auto caps = platform::getStackTraceCapabilities();
    VIXEN_ASSERT(caps.supportsStackTraceResolution);

    platform::resolveStackAddrs(*this->alloc, *this);
    return true;
}

} // namespace vixen
