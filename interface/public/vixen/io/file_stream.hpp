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

struct FileChunksSource {
    char const *path;
    View<char> buffer;
};

struct split_chunks_adapter {};

} // namespace placeholder

placeholder::FileChunksSource fileChunksSource(char const *path, View<char> buffer) {
    return placeholder::FileChunksSource{path, buffer};
}

placeholder::split_chunks_adapter split_chunks() {
    return placeholder::split_chunks_adapter{};
}

template <typename Sink>
struct split_chunks_adapter {
    Sink sink;

    template <typename I>
    void push(View<I> items) {
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
inline void operator>>=(vixen::stream::placeholder::FileChunksSource &&stream, Sink &&sink) {
    using namespace vixen;

    File file(stream.path, mode::READ);

    loop {
        auto len = file.read(stream.buffer);
        if (len == 0) break;
        sink.push(stream.buffer[rangeTo(len)]);
    };
}
