#pragma once

#include "vixen/io/file.hpp"
#include "vixen/util.hpp"

#include <fcntl.h>
#include <unistd.h>

namespace vixen {

inline bool OpenMode::isReadOnly() const {
    return read && !write;
}

inline bool OpenMode::isWriteOnly() const {
    return write && !read;
}

inline bool OpenMode::isReadWrite() const {
    return read && write;
}

// detail
inline int getOpenModeFlags(OpenMode mode) {
    int flags;

    if (mode.isReadOnly()) {
        flags = O_RDONLY;
    } else if (mode.isWriteOnly()) {
        flags = O_WRONLY;
    } else if (mode.isReadWrite()) {
        flags = O_RDWR;
    } else {
        VIXEN_PANIC("Tried to open file without read or write flags set.");
    }

    if (mode.append) flags |= O_APPEND;
    if (mode.truncate) flags |= O_TRUNC;
    if (mode.create) flags |= O_CREAT;

    return flags;
}

inline File::File(char const *path, OpenMode mode) {
    int flags = getOpenModeFlags(mode);
    fd = mode.create ? ::open(path, flags, mode.createMode) : ::open(path, flags);
}

inline File::~File() {
    ::close(fd);
}

inline usize File::read(View<char> buf) {
    return ::read(fd, buf.begin(), buf.len());
}

inline Vector<char> File::readAll(Allocator &alloc, usize chunkSize) {
    Vector<char> buf(alloc);

    loop {
        View<char> tmp{buf.reserve(chunkSize), chunkSize};
        usize bytesRead = read(tmp);

        if (bytesRead != chunkSize) {
            // Go one group back, which should be ok since we always reserve `chunkSize` each
            // iteration, so we can't underflow, and then forward the remaining bytes
            buf.truncate(buf.len() - chunkSize + bytesRead);
            break;
        }
    }

    return buf;
}

usize File::write(View<char> buf) {
    return ::write(fd, buf.begin(), buf.len());
}

void File::writeAll(View<char> buf) {
    // TODO: verify this is correct
    while (!buf.isEmpty()) {
        usize bytesWritten = write(buf);
        buf = buf[rangeFrom(bytesWritten)];
    }
}

} // namespace vixen

vixen::OpenMode operator|(vixen::OpenMode const &a, vixen::OpenMode const &b) {
    VIXEN_ASSERT(a.createMode == b.createMode);

    vixen::OpenMode n = a;
    n.read |= b.read;
    n.write |= b.write;
    n.append |= b.append;
    n.truncate |= b.truncate;

    if (b.create) {
        VIXEN_ASSERT(!n.create);
        n.create = true;
        n.createMode = b.createMode;
    }

    return n;
}
