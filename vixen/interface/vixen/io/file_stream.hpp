#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/allocator/allocators.hpp"
#include "vixen/io/file.hpp"
#include "vixen/slice.hpp"
#include "vixen/stream.hpp"
#include "vixen/string.hpp"
#include "vixen/util.hpp"

#include <fcntl.h>
#include <unistd.h>

namespace vixen::stream {

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

} // namespace vixen::stream

template <typename Sink>
inline vixen::stream::split_chunks_adapter<Sink> operator>>=(
    vixen::stream::placeholder::split_chunks_adapter &&stream, Sink &&sink) {
    return vixen::stream::split_chunks_adapter<Sink>{sink};
}

template <typename Sink>
inline void operator>>=(vixen::stream::placeholder::file_chunks_source &&stream, Sink &&sink) {
    using namespace vixen;

    vixen::file file(stream.path, vixen::mode::read);
    defer(destroy(file));

    loop {
        usize len = file.read_chunk(stream.buffer);
        sink.push(stream.buffer[vixen::range_to(len)]);
    }
}
