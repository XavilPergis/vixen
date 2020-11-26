#include "vixen/allocator/stacktrace.hpp"

#include "vixen/assert.hpp"
#include "vixen/defer.hpp"
#include "vixen/util.hpp"

#include <execinfo.h>

namespace vixen {

#pragma region "Internal"
// + ----- Internal ------------------------------------------------------------- +

namespace detail {
string get_command_output(allocator *alloc, string cmd) {
    vector<char> cmdbuf = MOVE(cmd.data);
    null_terminate(&cmdbuf);

    FILE *pipe = popen(cmdbuf.begin(), "r");
    if (pipe == nullptr) {
        VIXEN_ASSERT(false, "`popen({})` failed.", cmd);
    }
    defer(pclose(pipe));

    string result(alloc);
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result.push(buffer);
    }
    return result;
}

address_info translate_symbol(allocator *alloc, string_slice sym) {
    usize last_open_paren = *sym.last_index_of('(');
    usize last_close_paren = *sym.last_index_of(')');

    string_slice addr = sym[range(last_open_paren + 1, last_close_paren)];
    string_slice exepath = sym[range_to(last_open_paren)];

    string cmd(alloc);
    cmd.push("addr2line -fCe "_s);
    cmd.push(exepath);
    cmd.push(" "_s);
    cmd.push(addr);

    string output = get_command_output(alloc, MOVE(cmd));
    vector<string> lines = output.as_slice().split(alloc, "\n"_s);
    if (lines.len() < 2) {
        VIXEN_ASSERT(false, "addr2line failed with output: `{}`", output);
    }

    address_info info;
    info.name = MOVE(lines[0]);
    info.location = MOVE(lines[1]);
    info.raw_symbol = string(alloc, sym);
    return info;
}

vector<string> translate_addrs(allocator *alloc, slice<void *> addrs) {
    char **syms = backtrace_symbols(addrs.ptr, addrs.len);
    if (syms == nullptr) {
        VIXEN_ASSERT(false, "`backtrace_symbols` name buffer allocation failed.");
    }
    defer(free((void *)syms));

    vector<string> ret(alloc);
    for (usize i = 0; i < addrs.len; ++i) {
        ret.push(string(alloc, syms[i]));
    }
    return ret;
}

} // namespace detail

#pragma endregion
#pragma region "address_info"
// + ----- address_info ---------------------------------------------------------- +

address_info::address_info(allocator *alloc) : name(alloc), location(alloc), raw_symbol(alloc) {}
address_info::address_info(allocator *alloc, const address_info &other)
    : name(other.name.clone(alloc))
    , location(other.location.clone(alloc))
    , raw_symbol(other.raw_symbol.clone(alloc)) {}

address_info address_info::clone(allocator *alloc) const {
    return address_info(alloc, *this);
}

#pragma endregion
#pragma region "translation_cache"
// + ----- translation_cache ----------------------------------------------------- +

translation_cache::translation_cache(allocator *alloc)
    : alloc(alloc), translated(alloc), translation_queue(alloc) {}

translation_cache::translation_cache(allocator *alloc, const translation_cache &other)
    : alloc(alloc)
    , translated(alloc, other.translated)
    , translation_queue(alloc, other.translation_queue) {}

void translation_cache::add_symbol(void *symbol) {
    translation_queue.push(symbol);
}

void translation_cache::translate() {
    translation_queue.dedup_unstable();
    vector<string> symbols = detail::translate_addrs(alloc, translation_queue);

    for (usize i = 0; i < translation_queue.len(); ++i) {
        address_info info = detail::translate_symbol(alloc, symbols[i]);
        translated.insert(translation_queue[i], MOVE(info));
    }

    translation_queue.clear();
}

address_info const &translation_cache::get_info(void *addr) const {
    VIXEN_ASSERT(translated.key_exists(addr), "Address {} was not translated yet.", addr);
    return translated[addr];
}

#pragma endregion

vector<void *> capture_stack_trace(allocator *alloc) {
    // How much the stack buffer grows on each failed iteration attempt
    usize inc = 128;

    // let's *hope* that this only needs to loop once lmao
    for (usize current_depth = inc;; current_depth += inc) {
        vector<void *> stack = vector<void *>(alloc);
        usize actual_depth = backtrace(stack.reserve(current_depth), current_depth);

        // If the buffer wasn't entirely filled by backtrace, then we know there isn't any more
        // frames.
        if (actual_depth != current_depth) {
            stack.truncate(actual_depth);
            return stack;
        }
    }
}

vector<address_info> translate_stack_trace(translation_cache *cache, slice<void *> trace) {
    vector<address_info> infos(cache->alloc);

    for (void *addr : trace) {
        cache->add_symbol(addr);
    }

    cache->translate();
    for (void *addr : trace) {
        infos.push(cache->get_info(addr).clone(cache->alloc));
    }

    return infos;
}

void print_stack_trace_capture(slice<const address_info> info) {
    VIXEN_INFO("Stack was {} calls deep:", info.len);
    for (usize i = 0; i < info.len; ++i) {
        VIXEN_INFO("\x1b[36m{: >2}\x1b[0m :: \x1b[1m\x1b[32m{}\x1b[0m", i + 1, info[i].name);
        VIXEN_INFO("   :: in {}", info[i].location);
        VIXEN_INFO("   :: symbol {}", info[i].raw_symbol);
        VIXEN_INFO("   ==");
    }
}

void print_stack_trace() {
    vector<void *> trace = capture_stack_trace(heap::debug_allocator());
    translation_cache cache(heap::debug_allocator());
    vector<address_info> info = translate_stack_trace(&cache, trace);

    print_stack_trace_capture(info);
}

} // namespace vixen
