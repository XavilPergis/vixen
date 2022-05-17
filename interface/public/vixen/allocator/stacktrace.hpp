#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/hash/map.hpp"
#include "vixen/shared.hpp"
#include "vixen/string.hpp"
#include "vixen/traits.hpp"
#include "vixen/types.hpp"
#include "vixen/vector.hpp"

namespace vixen {

template <typename T>
T clone(Allocator &alloc, const T &value) {
    return T(copy_tag, alloc, value);
}

template <typename T>
Option<T> cloneOption(Allocator &alloc, const Option<T> &value) {
    Option<T> cloned;
    if (value.isSome()) {
        cloned = T(copy_tag, alloc, value.get());
    }
    return cloned;
}

struct SourceLocation {
    SourceLocation(copy_tag_t, Allocator &alloc, const SourceLocation &other)
        : line(other.line)
        , column(other.column)
        , resolvedFileName(String::copyFrom(alloc, other.resolvedFileName)) {}

    usize line = 0;
    usize column = 0;
    String resolvedFileName;
};

struct StackTrace {
    explicit StackTrace(Allocator &alloc) : callStackFrames(alloc) {}

    StackTrace(copy_tag_t, Allocator &alloc, const StackTrace &other)
        : callStackFrames(clone(alloc, other.callStackFrames)) {}

    static StackTrace captureCurrent(Allocator &alloc);

    struct Frame {
        explicit Frame(void *ip) : instructionPointer(ip) {}

        Frame(copy_tag_t, Allocator &alloc, const Frame &other)
            : instructionPointer(other.instructionPointer)
            , resolvedLocation(cloneOption(alloc, other.resolvedLocation))
            , resolvedFunctionName(cloneOption(alloc, other.resolvedFunctionName)) {}

        void *instructionPointer = nullptr;
        Option<SourceLocation> resolvedLocation;
        Option<String> resolvedFunctionName;
    };

    Vector<Frame> callStackFrames;
};

// void printCurrentStackTrace(Allocator &tempAlloc);
// void resolveStackTrace(Allocator &tempAlloc, StackTrace &stackTrace);
// void printStackTrace(Allocator &tempAlloc, const StackTrace &stackTrace);

template <typename I>
void formatFrame(I &oi, const StackTrace::Frame &frame, usize depth) {
    fmt::format_to(oi, "\x1b[32m{: >2} \x1b[90m❙ ", depth);
    fmt::format_to(oi, "{:p} \x1b[0m› ", frame.instructionPointer);

    // 0 ❙ 0x7fffffc7a34 › ??? • ???
    // 1 ❙ 0x7fffffc7a34 › /path/to/file.cpp:32:7 • ???
    // 2 ❙ 0x7fffffc7a34 › ??? • foo(int)
    // 3 ❙ 0x7fffffc7a34 › /path/to/file.cpp:32:7 • foo(int)

    if (frame.resolvedLocation) {
        fmt::format_to(oi,
            "\x1b[32m{}\x1b[0m:\x1b[1;32m{}\x1b[0m:\x1b[1;32m{}\x1b[0m",
            frame.resolvedLocation->resolvedFileName,
            frame.resolvedLocation->line,
            frame.resolvedLocation->column);
    } else {
        fmt::format_to(oi, "\x1b[90m???\x1b[0m");
    }
    fmt::format_to(oi, " \x1b[90m•\x1b[0m ");

    if (frame.resolvedFunctionName) {
        fmt::format_to(oi, "{}", frame.resolvedFunctionName.get());
    } else {
        fmt::format_to(oi, "\x1b[90m???\x1b[0m");
    }

    fmt::format_to(oi, "\n");
}

template <typename I>
void formatStackTrace(I &oi, const StackTrace &trace) {
    usize depth = 1;
    VIXEN_DEBUG("outer");
    for (const auto &frame : trace.callStackFrames) {
        VIXEN_DEBUG("inner");
        formatFrame(oi, frame, depth++);
    }
}

namespace platform {

struct StackTraceCaptureContext {
    usize framesToSkip = 0;
    usize framesToCapture = 0;
    void **outFrames = nullptr;
    usize *outFramesCount = nullptr;
};

struct StackTraceCapabilities {
    bool supportsStackTraceCapture = false;
    bool supportsStackTraceResolution = false;
};

StackTraceCapabilities getStackTraceCapabilities();
void captureStackFrames(StackTraceCaptureContext &opts);

} // namespace platform

} // namespace vixen
