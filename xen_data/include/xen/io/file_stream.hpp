#pragma once

#include "xen/allocator/allocator.hpp"
#include "xen/allocator/allocators.hpp"
#include "xen/io/file.hpp"
#include "xen/stream.hpp"
#include "xen/string.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <xen/slice.hpp>
#include <xen/util.hpp>

namespace xen::stream {

namespace placeholder {

struct file_chunks_source {
    char const *path;
    slice<char> buffer;
};

struct split_chunks_adapter {};

} // namespace placeholder

placeholder::file_chunks_source file_chunks_source(char const *path, slice<char> buffer) {
    return placeholder::file_chunks_source{path, buffer};
}

placeholder::split_chunks_adapter split_chunks() {
    return placeholder::split_chunks_adapter{};
}

template <typename Sink>
struct split_chunks_adapter {
    Sink sink;

    template <typename I>
    void push(slice<I> items) {
        for (I &item : items) {
            sink.push(item);
        }
    }
};

} // namespace xen::stream

template <typename Sink>
inline xen::stream::split_chunks_adapter<Sink> operator>>=(
    xen::stream::placeholder::split_chunks_adapter &&stream, Sink &&sink) {
    return xen::stream::split_chunks_adapter<Sink>{sink};
}

template <typename Sink>
inline void operator>>=(xen::stream::placeholder::file_chunks_source &&stream, Sink &&sink) {
    using namespace xen;

    xen::file file(stream.path, xen::mode::read);
    defer(destroy(file));

    loop {
        usize len = file.read_chunk(stream.buffer);
        sink.push(stream.buffer[xen::range_to(len)]);
    }
}
