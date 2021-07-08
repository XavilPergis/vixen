#include "vixen/allocator/stacktrace.hpp"

#include "vixen/assert.hpp"
#include "vixen/defer.hpp"
#include "vixen/util.hpp"

#include <dlfcn.h>
#include <execinfo.h>

namespace vixen {

#pragma region "Internal"
// + ----- Internal ------------------------------------------------------------- +

namespace detail {
String get_command_output(Allocator *alloc, String cmd) {
    Vector<char> cmdbuf = mv(cmd.data);
    null_terminate(cmdbuf);

    FILE *pipe = popen(cmdbuf.begin(), "r");
    if (pipe == nullptr) {
        VIXEN_PANIC("`popen({})` failed.", cmd);
    }
    defer(pclose(pipe));

    String result(alloc);
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result.push(buffer);
    }
    return result;
}

template <typename T>
String format_string(Allocator *alloc, const T &value) {
    Vector<char> diagnostic(alloc);
    auto bi = stream::back_inserter(diagnostic);
    auto oi = stream::make_stream_output_iterator(bi);
    fmt::format_to(oi, "{}", value);
    return String(mv(diagnostic));
}

AddressInfo translate_symbol(Allocator *alloc, StringView sym, void *reladdr) {
    auto open_paren = sym.index_of('(');
    auto open_brace = sym.last_index_of('[');
    auto close_brace = sym.last_index_of(']');

    AddressInfo info;
    info.raw_symbol = String(alloc, sym);
    if (open_paren.isNone()) {
        info.name = String(alloc, "??"_s);
        info.location = String(alloc, "??"_s);
        return info;
    }

    StringView exepath = sym[range_to(*open_paren)];

    VIXEN_ASSERT(open_brace.isSome() && close_brace.isSome());
    VIXEN_ASSERT(open_brace.get() < sym.len());
    StringView addr = sym[range(open_brace.get() + 1, close_brace.get())];

    String cmd(alloc);
    cmd.push("addr2line -ifCe "_s);
    cmd.push(exepath);
    cmd.push(" "_s);
    cmd.push(addr);

    String output = get_command_output(alloc, mv(cmd));
    Vector<String> lines = output.as_slice().split(alloc, "\n"_s);
    if (lines.len() < 2) {
        VIXEN_PANIC("addr2line failed with output: `{}`", output);
    }

    info.name = mv(lines[0]);
    info.location = mv(lines[1]);
    return info;
}

void *get_reladdr(void *absolute) {
    Dl_info info;
    dladdr(absolute, &info);
    return (void *)((usize)absolute - (usize)info.dli_fbase);
}

Vector<String> translate_addrs(Allocator *alloc, Slice<void *> addrs) {
    char **syms = backtrace_symbols(addrs.begin(), addrs.len());
    if (syms == nullptr) {
        VIXEN_PANIC("`backtrace_symbols` name buffer allocation failed.");
    }
    defer(free((void *)syms));

    Vector<String> ret(alloc);
    for (usize i = 0; i < addrs.len(); ++i) {
        ret.push(String(alloc, syms[i]));
    }
    return ret;
}

} // namespace detail

#pragma endregion
#pragma region "address_info"
// + ----- AddressInfo ---------------------------------------------------------- +

AddressInfo::AddressInfo(Allocator *alloc) : name(alloc), location(alloc), raw_symbol(alloc) {}
AddressInfo::AddressInfo(copy_tag_t, Allocator *alloc, const AddressInfo &other)
    : name(other.name.clone(alloc))
    , location(other.location.clone(alloc))
    , raw_symbol(other.raw_symbol.clone(alloc)) {}

#pragma endregion
#pragma region "translation_cache"
// + ----- TranslationCache ----------------------------------------------------- +

TranslationCache::TranslationCache(Allocator *alloc)
    : alloc(alloc), translation_queue(alloc), translated(alloc) {}

TranslationCache::TranslationCache(copy_tag_t, Allocator *alloc, const TranslationCache &other)
    : alloc(alloc)
    , translation_queue(other.translation_queue.clone(alloc))
    , translated(other.translated.clone(alloc)) {}

void TranslationCache::add_symbol(void *symbol) {
    translation_queue.push(symbol);
}

void TranslationCache::translate() {
    translation_queue.dedupUnstable();
    Vector<String> symbols = detail::translate_addrs(alloc, translation_queue);

    for (usize i = 0; i < translation_queue.len(); ++i) {
        AddressInfo info = detail::translate_symbol(alloc,
            symbols[i],
            detail::get_reladdr(translation_queue[i]));
        translated.insert(translation_queue[i], mv(info));
    }

    translation_queue.clear();
}

AddressInfo const &TranslationCache::get_info(void *addr) const {
    VIXEN_ASSERT_EXT(translated.contains_key(addr), "Address {} was not translated yet.", addr);
    return translated[addr];
}

#pragma endregion

Vector<void *> capture_stack_trace(Allocator *alloc) {
    // How much the stack buffer grows on each failed iteration attempt
    usize inc = 128;

    // let's *hope* that this only needs to loop once lmao
    for (usize current_depth = inc;; current_depth += inc) {
        Vector<void *> stack = Vector<void *>(alloc);
        usize actual_depth = backtrace(stack.reserve(current_depth), current_depth);

        // If the buffer wasn't entirely filled by backtrace, then we know there isn't any more
        // frames.
        if (actual_depth != current_depth) {
            stack.truncate(actual_depth);
            return stack;
        }
    }
}

Vector<AddressInfo> translate_stack_trace(TranslationCache *cache, Slice<void *> trace) {
    Vector<AddressInfo> infos(cache->alloc);

    for (void *addr : trace) {
        cache->add_symbol(addr);
    }

    cache->translate();
    for (void *addr : trace) {
        infos.push(cache->get_info(addr).clone(cache->alloc));
    }

    return infos;
}

void print_stack_trace_capture(Slice<const AddressInfo> info) {
    VIXEN_INFO("Stack was {} calls deep:", info.len());
    for (usize i = 0; i < info.len(); ++i) {
        VIXEN_INFO("\x1b[36m{: >2}\x1b[0m :: \x1b[1m\x1b[32m{}\x1b[0m", i + 1, info[i].name);
        VIXEN_INFO("   :: in {}", info[i].location);
        VIXEN_INFO("   :: symbol \x1b[30m{}\x1b[0m", info[i].raw_symbol);
        VIXEN_INFO("   ==");
    }
}

void print_stack_trace() {
    Vector<void *> trace = capture_stack_trace(heap::debug_allocator());
    TranslationCache cache(heap::debug_allocator());
    Vector<AddressInfo> info = translate_stack_trace(&cache, trace);

    print_stack_trace_capture(info);
}

} // namespace vixen
