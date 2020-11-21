#pragma once

#include "xen/io/file.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <xen/util.hpp>

namespace xen {

bool open_mode::is_readonly() const {
    return read && !write;
}

bool open_mode::is_writeonly() const {
    return write && !read;
}

bool open_mode::is_readwrite() const {
    return read && write;
}

// detail
inline int get_open_mode_flags(open_mode mode) {
    int flags;

    if (mode.is_readonly()) {
        flags = O_RDONLY;
    } else if (mode.is_writeonly()) {
        flags = O_WRONLY;
    } else if (mode.is_readwrite()) {
        flags = O_RDWR;
    } else {
        XEN_ASSERT(false, "Tried to open file without read or write flags set.")
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

inline file::file(char const *path, open_mode mode) {
    int flags = get_open_mode_flags(mode);
    fd = mode.create ? ::open(path, flags, mode.create_mode) : ::open(path, flags);
}

inline file::~file() {
    ::close(fd);
}

inline usize file::read_chunk(slice<char> buf) {
    return ::read(fd, buf.ptr, buf.len);
}

inline vector<char> file::read_all(allocator *alloc, usize chunk_size) {
    vector<char> buf(alloc);

    loop {
        slice<char> tmp{buf.reserve(chunk_size), chunk_size};
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

} // namespace xen

xen::open_mode operator|(xen::open_mode const &a, xen::open_mode const &b) {
    XEN_ENGINE_ASSERT(a.create_mode == b.create_mode, "")

    xen::open_mode n = a;
    n.read |= b.read;
    n.write |= b.write;
    n.append |= b.append;
    n.truncate |= b.truncate;

    if (b.create) {
        XEN_ENGINE_ASSERT(!n.create, "")
        n.create = true;
        n.create_mode = b.create_mode;
    }

    return n;
}
