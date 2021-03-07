#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/hash/map.hpp"
#include "vixen/string.hpp"
#include "vixen/traits.hpp"
#include "vixen/types.hpp"
#include "vixen/vec.hpp"

namespace vixen {
struct AddressInfo {
    String name;
    String location;
    String raw_symbol;

    AddressInfo() = default;

    AddressInfo(AddressInfo &&other) = default;
    AddressInfo &operator=(AddressInfo &&other) = default;

    explicit AddressInfo(Allocator *alloc);
    AddressInfo(copy_tag_t, Allocator *alloc, const AddressInfo &other);

    VIXEN_DEFINE_CLONE_METHOD(AddressInfo)
};

struct TranslationCache {
    Allocator *alloc;
    Vector<void *> translation_queue;
    HashMap<void *, AddressInfo> translated;

    TranslationCache(TranslationCache &&other) = default;
    TranslationCache &operator=(TranslationCache &&other) = default;

    explicit TranslationCache(Allocator *alloc);
    TranslationCache(copy_tag_t, Allocator *alloc, const TranslationCache &other);

    VIXEN_DEFINE_CLONE_METHOD(TranslationCache)

    void add_symbol(void *symbol);
    void translate();
    AddressInfo const &get_info(void *symbol) const;
};

Vector<void *> capture_stack_trace(Allocator *alloc);

Vector<AddressInfo> translate_stack_trace(TranslationCache *cache, Slice<void *> trace);
void print_stack_trace_capture(Slice<const AddressInfo> info);

void print_stack_trace();

template <typename Stream>
void format_address_info(Stream &stream, const AddressInfo &info, usize level) {
    auto oi = stream::make_stream_output_iterator(stream);
    fmt::format_to(oi, "\x1b[36m{: >2}\x1b[0m :: \x1b[1m\x1b[32m{}\x1b[0m\n", level, info.name);
    fmt::format_to(oi, "   :: in {}\n", info.location);
    fmt::format_to(oi, "   :: symbol \x1b[30m{}\x1b[0m\n", info.raw_symbol);
    fmt::format_to(oi, "\n");
}

} // namespace vixen
