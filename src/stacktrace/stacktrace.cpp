#include "vixen/allocator/stacktrace.hpp"

#include "vixen/assert.hpp"
#include "vixen/defer.hpp"
#include "vixen/util.hpp"

namespace vixen {

StackTrace StackTrace::captureCurrent(Allocator &alloc) {
    auto caps = platform::getStackTraceCapabilities();

    // TODO: handle this better
    VIXEN_ASSERT(caps.supportsStackTraceCapture);

    // How much the stack buffer grows on each failed iteration attempt
    usize traceSizeInitial = 64;
    usize traceSizeIncrement = 64;

    Vector<void *> frames(alloc);
    StackTrace trace(alloc);
    for (usize requestedFrameCount = traceSizeInitial;; requestedFrameCount += traceSizeIncrement) {
        usize frameCount = 0;
        frames.removeAll();

        platform::StackTraceCaptureContext opts{};
        opts.framesToCapture = requestedFrameCount;
        // FIXME
        // opts.outFrames = frames.reserve(requestedFrameCount);
        opts.outFramesCount = &frameCount;

        platform::captureStackFrames(opts);

        // If the buffer wasn't entirely filled by backtrace, then we know there isn't any more
        // frames.
        if (frameCount != requestedFrameCount) {
            frames.shrinkTo(frameCount);
            for (usize i = 0; i < frames.len(); ++i) {
                trace.callStackFrames.emplaceLast(frames[i]);
            }
            break;
        }
    }

    return trace;
}

} // namespace vixen
