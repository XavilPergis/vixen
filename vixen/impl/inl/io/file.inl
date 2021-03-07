#pragma once

#include "vixen/io/file.hpp"
#include "vixen/util.hpp"

#include <fcntl.h>
#include <unistd.h>

namespace vixen {

inline bool OpenMode::is_readonly() const {
    return read && !write;
}

inline bool OpenMode::is_writeonly() const {
    return write && !read;
}

inline bool OpenMode::is_readwrite() const {
    return read && write;
}

// detail
inline int get_open_mode_flags(OpenMode mode) {
    int flags;

    if (mode.is_readonly()) {
        flags = O_RDONLY;
    } else if (mode.is_writeonly()) {
        flags = O_WRONLY;
    } else if (mode.is_readwrite()) {
        flags = O_RDWR;
    } else {
        VIXEN_PANIC("Tried to open file without read or write flags set.");
    }

    if (mode.append) {
        flags |= O_APPEND;
    }
    if (mode.truncate) {
        flags |= O_TRUNC;
    }
    if (mode.create) {
        flags |= O_CREAT;
    }

    return flags;
}

inline File::File(char const *path, OpenMode mode) {
    int flags = get_open_mode_flags(mode);
    fd = mode.create ? ::open(path, flags, mode.create_mode) : ::open(path, flags);
}

inline File::~File() {
    ::close(fd);
}

inline usize File::read_chunk(Slice<char> buf) {
    return ::read(fd, buf.begin(), buf.len());
}

inline Vector<char> File::read_all(Allocator *alloc, usize chunk_size) {
    Vector<char> buf(alloc);

    loop {
        Slice<char> tmp{buf.reserve(chunk_size), chunk_size};
        usize bytes_read = read_chunk(tmp);

        if (bytes_read != chunk_size) {
            // Go one group back, which should be ok since we always reserve `chunk_size` each
            // iteration, so we can't underflow, and then forward the remaining bytes
            buf.truncate(buf.len() - chunk_size + bytes_read);
            break;
        }
    }

    return buf;
}

} // namespace vixen

vixen::OpenMode operator|(vixen::OpenMode const &a, vixen::OpenMode const &b) {
    VIXEN_ASSERT(a.create_mode == b.create_mode);

    vixen::OpenMode n = a;
    n.read |= b.read;
    n.write |= b.write;
    n.append |= b.append;
    n.truncate |= b.truncate;

    if (b.create) {
        VIXEN_ASSERT(!n.create);
        n.create = true;
        n.create_mode = b.create_mode;
    }

    return n;
}
