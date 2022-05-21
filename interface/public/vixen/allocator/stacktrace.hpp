#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/hash/map.hpp"
#include "vixen/shared.hpp"
#include "vixen/string.hpp"
#include "vixen/traits.hpp"
#include "vixen/types.hpp"
#include "vixen/vector.hpp"

namespace vixen {

struct SourceLocation {
    SourceLocation() = default;
    SourceLocation(copy_tag_t, Allocator &alloc, const SourceLocation &other)
        : line(other.line)
        , column(other.column)
        , resolvedFilePath(String::copyFrom(alloc, other.resolvedFilePath)) {}
    VIXEN_DEFINE_CLONE_METHOD(SourceLocation)

    usize line = 0;
    Option<usize> column{};
    String resolvedFilePath;
    Option<String> resolvedBinaryPath;
};

struct CaptureOptions {
    /**
     * @brief if true, frames related to capturing the stacktrace are not included in the trace.
     */
    bool skipSelfFrames = true;

    /**
     * @brief if true, frames before 'main' are omitted.
     */
    bool skipBeforeMainFrames = true;

    /**
     * @brief the number of frames to omit.
     */
    usize skipFrames = 0;

    /**
     * @brief the number of frames to capture, or an empty option for all frames.
     */
    Option<usize> captureFrames{};
};

struct StackTrace {
    explicit StackTrace(Allocator &alloc) : alloc(&alloc), callStackFrames(alloc) {}

    StackTrace(copy_tag_t, Allocator &alloc, const StackTrace &other)
        : callStackFrames(other.callStackFrames.clone(alloc)) {}
    VIXEN_DEFINE_CLONE_METHOD(StackTrace)

    static StackTrace captureCurrent(Allocators alloc, CaptureOptions opts = CaptureOptions{});
    bool resolveSourceLocations();

    struct Frame {
        explicit Frame(uintptr ip) : instructionPointer(ip) {}

        Frame(copy_tag_t, Allocator &alloc, const Frame &other)
            : instructionPointer(other.instructionPointer)
            , resolvedLocation(other.resolvedLocation.clone(alloc))
            , resolvedFunctionName(other.resolvedFunctionName.clone(alloc)) {}
        VIXEN_DEFINE_CLONE_METHOD(Frame)

        bool omitted = false;
        uintptr instructionPointer = 0;
        Option<SourceLocation> resolvedLocation{};
        Option<String> resolvedFunctionName{};
    };

    Allocator *alloc;
    /**
     * @brief the stack backtrace, more recent calls come first.
     */
    Vector<Frame> callStackFrames{};
    bool wereRecentFramesOmitted = false;
    bool wereOldFramesOmitted = false;
};

// void printCurrentStackTrace(Allocator &tempAlloc);
// void resolveStackTrace(Allocator &tempAlloc, StackTrace &stackTrace);
// void printStackTrace(Allocator &tempAlloc, const StackTrace &stackTrace);

template <typename I>
void formatFrame(I &oi, const StackTrace::Frame &frame, usize depth) {
    // fmt::format_to(oi, "\x1b[32m{: >2} \x1b[90m❙ ", depth);
    fmt::format_to(oi, "\x1b[32m{: >3} \x1b[0m| ", depth);
    // fmt::format_to(oi, "{:x} \x1b[0m› ", frame.instructionPointer);
    fmt::format_to(oi, "\x1b[90m{:x} \x1b[0m> ", frame.instructionPointer);

    // 0 ❙ 0x7fffffc7a34 › ??? • ???
    // 1 ❙ 0x7fffffc7a34 › /path/to/file.cpp:32:7 • ???
    // 2 ❙ 0x7fffffc7a34 › ??? • foo(int)
    // 3 ❙ 0x7fffffc7a34 › /path/to/file.cpp:32:7 • foo(int)

    // 0 | 0x7fffffc7a34 > ???
    // 1 | 0x7fffffc7a34 > ???
    //     -> /path/to/file.cpp:32:7
    // 2 | 0x7fffffc7a34 > foo(int)
    // 3 | 0x7fffffc7a34 > foo(int)
    //     -> /path/to/file.cpp:32:7

    if (frame.resolvedFunctionName) {
        fmt::format_to(oi, "\x1b[32m{}\x1b[0m", frame.resolvedFunctionName.get());
    } else {
        fmt::format_to(oi, "\x1b[90m???\x1b[0m");
    }

    if (frame.resolvedLocation) {
        fmt::format_to(oi, "\n      \x1b[1m-\x1b[0m ");
        fmt::format_to(oi,
            "{}:\x1b[1;90m{}\x1b[0m",
            frame.resolvedLocation->resolvedFilePath,
            frame.resolvedLocation->line);

        if (frame.resolvedLocation->column.isSome()) {
            fmt::format_to(oi, ":\x1b[1;90m{}\x1b[0m", frame.resolvedLocation->column.get());
        }

        if (frame.resolvedLocation->resolvedBinaryPath.isSome()) {
            fmt::format_to(oi, "\n      \x1b[1m-\x1b[0m ");
            fmt::format_to(oi,
                "\x1b[90m{}\x1b[0m",
                frame.resolvedLocation->resolvedBinaryPath.get());
        }
    }

    // fmt::format_to(oi, " \x1b[90m•\x1b[0m ");
    // fmt::format_to(oi, " \x1b[90m\x1b[0m ");

    fmt::format_to(oi, "\n");
}

template <typename I>
void formatStackTrace(I &oi, const StackTrace &trace) {
    usize depth = 1;

    usize startIndex = 0;
    for (usize i = 0; i < trace.callStackFrames.len(); ++i) {
        auto const &frame = trace.callStackFrames[i];
        if (frame.resolvedFunctionName.isSome() and frame.resolvedFunctionName.get() == "main") {
            startIndex = i;
            depth += i;
            break;
        }
    }

    if (startIndex != 0) {
        fmt::format_to(oi, "\x1b[32m{: >3} \x1b[0m| \x1b[90m...\x1b[0m\n", depth - 1);
        // fmt::format_to(oi, "\x1b[90m[more calls]\x1b[0m\n");
    }

    for (usize i = startIndex; i < trace.callStackFrames.len(); ++i) {
        auto const &frame = trace.callStackFrames[i];
        formatFrame(oi, frame, depth++);
    }

    if (trace.wereRecentFramesOmitted) {
        fmt::format_to(oi, "\x1b[32m{: >3} \x1b[0m| \x1b[90m...\x1b[0m\n", depth);
        // fmt::format_to(oi, "\x1b[90m[more calls]\x1b[0m\n");
    }
}

namespace platform {

struct StackTraceAddrsCaptureContext {
    bool skipSelfFrames = true;
    usize framesToSkip = 0;
    Option<usize> maxFramesToCapture{};
    Allocators allocators{};

    bool outWasCaptureSuccessful = false;
    StackTrace *out;
};

struct StackTraceCapabilities {
    bool supportsStackTraceCapture = false;
    bool supportsStackTraceResolution = false;
};

StackTraceCapabilities getStackTraceCapabilities();
bool captureStackAddrs(StackTrace &outTrace, StackTraceAddrsCaptureContext &opts);
bool resolveStackAddrs(Allocators alloc, StackTrace &outTrace);

} // namespace platform

} // namespace vixen
