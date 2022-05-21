#include "vixen/allocator/stacktrace.hpp"
#include "vixen/assert.hpp"

#if defined(VIXEN_PLATFORM_WINDOWS)
#define _AMD64_
#include <windows.h>

#include <windef.h>

#include <dbghelp.h>
#include <psapi.h>
#include <winnt.h>
#endif

namespace vixen::platform {

StackTraceCapabilities getStackTraceCapabilities() {
    StackTraceCapabilities caps{};

#if defined(VIXEN_PLATFORM_WINDOWS)
    caps.supportsStackTraceCapture = true;
    caps.supportsStackTraceResolution = true;
#endif

    return caps;
}

#if defined(VIXEN_PLATFORM_WINDOWS)

struct Sym {
    static Sym &instance() {
        static Sym gInstance{};
        if (!gInstance.mInitialized) gInstance.initialize();
        return gInstance;
    }

    static bool ensureInitialized() {
        Sym &sym = instance();
        sym.initialize();
        return sym.mInitialized;
    }

    BOOL getLineFromAddr(DWORD64 qwAddr, PDWORD pdwDisplacement, PIMAGEHLP_LINE64 Line64) {
        return SymGetLineFromAddr(mProcess, qwAddr, pdwDisplacement, Line64);
    }

    DWORD64 getModuleBase(DWORD64 qwAddr) { return SymGetModuleBase(mProcess, qwAddr); }
    BOOL fromAddr(DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFO Symbol) {
        return SymFromAddr(mProcess, Address, Displacement, Symbol);
    }

private:
    Sym() : mProcess(GetCurrentProcess()) {}
    ~Sym() {
        if (mInitialized) SymCleanup(mProcess);
    }

    void initialize() {
        if (SymInitialize(mProcess, nullptr, true)) mInitialized = true;
        SymSetOptions(SYMOPT_LOAD_LINES);
    }

    static Sym sInstance;
    bool mInitialized;
    HANDLE mProcess;
};

String getModuleFileName(Allocator &alloc, HMODULE module) {
    String outString(alloc);

    auto amountCopied = 0;
    do {
        outString.reserveLast(MAX_PATH);
        amountCopied = GetModuleFileNameA(module, outString.begin(), outString.capacity());
        outString.unsafeGrowBy(amountCopied);
    } while (amountCopied == static_cast<isize>(outString.capacity()));

    outString.shrinkToFit();
    return mv(outString);
}

Option<String> getSymbolName(Allocators alloc, uintptr addr) {
    Sym &sym = Sym::instance();
    String outString(alloc.persistent());

    usize currentNameSize = MAX_SYM_NAME;
    while (true) {
        auto bufLen = sizeof(SYMBOL_INFO) + currentNameSize * sizeof(TCHAR);
        char *buffer = heap::createByteArrayUninit(alloc.temporary(), bufLen);
        defer(heap::destroyByteArrayUninit(alloc.temporary(), buffer, bufLen));

        PSYMBOL_INFO symbol = new (buffer) SYMBOL_INFO{};
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = currentNameSize;
        if (!sym.fromAddr(addr, nullptr, symbol)) {
            return {};
        }

        if (symbol->NameLen < currentNameSize) {
            char *outData = outString.reserveLast(symbol->NameLen);
            util::arrayCopyNonoverlapping(static_cast<char *>(symbol->Name),
                outData,
                symbol->NameLen);
            outString.unsafeGrowBy(symbol->NameLen);
            break;
        }

        currentNameSize += MAX_SYM_NAME;
    };

    outString.shrinkToFit();
    return mv(outString);
}

Option<usize> getLineNumber(uintptr addr) {
    Sym &sym = Sym::instance();

    IMAGEHLP_LINE line{};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    DWORD displacement;
    if (!sym.getLineFromAddr(addr, &displacement, &line)) {
        // FIXME: handle other classes of errors instead of just saying there was no line number
        // mapping.
        return {};
    }

    return line.LineNumber;
}

Option<String> getSourceFileName(Allocator &persistentAlloc, uintptr addr) {
    Sym &sym = Sym::instance();

    IMAGEHLP_LINE line{};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    DWORD displacement;
    if (!sym.getLineFromAddr(addr, &displacement, &line)) {
        // FIXME: handle other classes of errors instead of just saying there was no line number
        // mapping.
        return {};
    }

    return String::copyFromAsciiNullTerminated(persistentAlloc, line.FileName);
}

// Option<usize> getLineNumber(HANDLE process, uintptr addr) {
//     // HANDLE hProcess,DWORD64 qwAddr,PDWORD pdwDisplacement,PIMAGEHLP_LINE64 Line64
//     IMAGEHLP_LINE line{};
//     line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
//     if (!SymGetLineFromAddr(process, addr, nullptr, &line)) {
//         // FIXME: handle other classes of errors instead of just saying there was no line number
//         // mapping.
//         return {};
//     }

//     return line.LineNumber;
// }

bool captureStackAddrs(StackTrace &outTrace, StackTraceAddrsCaptureContext &opts) {
    if (!Sym::ensureInitialized()) return false;

    DWORD machine = IMAGE_FILE_MACHINE_AMD64;
    HANDLE currentProcess = GetCurrentProcess();
    HANDLE currentThread = GetCurrentThread();

    CONTEXT context{};
    context.ContextFlags = CONTEXT_FULL;
    RtlCaptureContext(&context);

    STACKFRAME frame = {};
    frame.AddrPC.Offset = context.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Rsp;
    frame.AddrStack.Mode = AddrModeFlat;

    usize remainingFramesToSkip = (opts.skipSelfFrames ? 1 : 0) + opts.framesToSkip;
    Option<usize> remainingFramesToCapture = opts.maxFramesToCapture;

    outTrace.wereRecentFramesOmitted = remainingFramesToSkip > 0;

    while (StackWalk(machine,
        currentProcess,
        currentThread,
        &frame,
        &context,
        nullptr,
        SymFunctionTableAccess,
        SymGetModuleBase,
        nullptr))
    {
        if (remainingFramesToSkip > 0) {
            remainingFramesToSkip -= 1;
            continue;
        }

        if (remainingFramesToCapture.isSome()) {
            if (remainingFramesToCapture.get() > 0) {
                remainingFramesToCapture.get() -= 1;
            } else {
                break;
            }
        }

        auto ip = static_cast<uintptr>(frame.AddrPC.Offset);
        outTrace.callStackFrames.insertLast(StackTrace::Frame(ip));
    }

    return true;
}

bool resolveStackAddrs(Allocators alloc, StackTrace &outTrace) {
    if (!Sym::ensureInitialized()) return false;
    Sym &sym = Sym::instance();

    for (StackTrace::Frame &frame : outTrace.callStackFrames) {
        frame.resolvedFunctionName = getSymbolName(alloc, frame.instructionPointer);

        auto moduleBase = sym.getModuleBase(frame.instructionPointer);
        auto binaryFilePath = getModuleFileName(alloc.persistent(), (HMODULE)moduleBase);
        auto sourceLineNumber = getLineNumber(frame.instructionPointer);
        auto sourceFilePath = getSourceFileName(alloc.persistent(), frame.instructionPointer);

        if (sourceFilePath.isSome() and sourceLineNumber.isSome()) {
            SourceLocation location{};
            location.resolvedFilePath = mv(sourceFilePath.get());
            location.line = sourceLineNumber.get();
            location.resolvedBinaryPath = mv(binaryFilePath);

            frame.resolvedLocation = mv(location);
        }
    }

    return true;
}
#endif

} // namespace vixen::platform
