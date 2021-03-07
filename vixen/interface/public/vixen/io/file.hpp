#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/slice.hpp"
#include "vixen/types.hpp"
#include "vixen/vec.hpp"

namespace vixen {

struct OpenMode {
    bool read, write, create, append, truncate;
    u16 create_mode;

    bool is_readonly() const;
    bool is_writeonly() const;
    bool is_readwrite() const;
};

namespace mode {
constexpr OpenMode read = {true, false, false, false, false, 0};
constexpr OpenMode write = {false, true, false, false, false, 0};
constexpr OpenMode append = {false, false, false, true, false, 0};
constexpr OpenMode truncate = {false, false, false, false, true, 0};
constexpr OpenMode create(u16 mode) {
    return {false, false, true, false, false, mode};
}
} // namespace mode

struct File {
    int fd;

    explicit File(char const *path, OpenMode mode);
    ~File();

    usize read_chunk(Slice<char> buf);
    Vector<char> read_all(Allocator *alloc, usize chunk_size = 512);

    usize write_chunk(Slice<char> buf);
    void write_all(Slice<char> buf);

    void flush();
};

} // namespace vixen

inline vixen::OpenMode operator|(vixen::OpenMode const &a, vixen::OpenMode const &b);

#include "io/file.inl"
