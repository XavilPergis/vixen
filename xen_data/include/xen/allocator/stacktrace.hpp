#pragma once

#include "xen/allocator/allocator.hpp"
#include "xen/hashmap.hpp"
#include "xen/string.hpp"
#include "xen/vec.hpp"

#include <xen/traits.hpp>
#include <xen/types.hpp>

namespace xen {
struct address_info {
    string name;
    string location;
    string raw_symbol;

    address_info() = default;

    address_info(address_info &&other) = default;
    address_info &operator=(address_info &&other) = default;

    explicit address_info(allocator *alloc);
    address_info(allocator *alloc, const address_info &other);
    address_info clone(allocator *alloc) const;
};

struct translation_cache {
    allocator *alloc;
    vector<void *> translation_queue;
    hash_map<void *, address_info> translated;

    translation_cache(translation_cache &&other) = default;
    translation_cache &operator=(translation_cache &&other) = default;

    explicit translation_cache(allocator *alloc);
    translation_cache(allocator *alloc, const translation_cache &other);
    translation_cache clone(allocator *alloc) const;

    void add_symbol(void *symbol);
    void translate();
    address_info const &get_info(void *symbol) const;
};

vector<void *> capture_stack_trace(allocator *alloc);

vector<address_info> translate_stack_trace(translation_cache *cache, slice<void *> trace);
void print_stack_trace_capture(slice<const address_info> info);

void print_stack_trace();

} // namespace xen
