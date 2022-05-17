#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/slice.hpp"
#include "vixen/types.hpp"
#include "vixen/vector.hpp"

namespace vixen {

struct OpenMode {
    bool read, write, create, append, truncate;
    u16 createMode;

    bool isReadOnly() const;
    bool isWriteOnly() const;
    bool isReadWrite() const;
};

namespace mode {
constexpr OpenMode READ = {true, false, false, false, false, 0};
constexpr OpenMode WRITE = {false, true, false, false, false, 0};
constexpr OpenMode APPEND = {false, false, false, true, false, 0};
constexpr OpenMode TRUNCATE = {false, false, false, false, true, 0};
constexpr OpenMode create(u16 mode) {
    return {false, false, true, false, false, mode};
}
} // namespace mode

struct File {
    int fd;

    explicit File(char const *path, OpenMode mode);
    ~File();

    usize read(View<char> buf);
    Vector<char> readAll(Allocator &alloc, usize chunkSize = 1024);

    usize write(View<char> buf);
    void writeAll(View<char> buf);

    void flush();
};

} // namespace vixen

inline vixen::OpenMode operator|(vixen::OpenMode const &a, vixen::OpenMode const &b);

#include "io/file.inl"
