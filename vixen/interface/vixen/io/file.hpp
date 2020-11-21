#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/slice.hpp"
#include "vixen/types.hpp"
#include "vixen/vec.hpp"

namespace vixen {

struct open_mode {
    bool read, write, create, append, truncate;
    u16 create_mode;

    bool is_readonly() const;
    bool is_writeonly() const;
    bool is_readwrite() const;
};

namespace mode {
constexpr open_mode read = {true, false, false, false, false, 0};
constexpr open_mode write = {false, true, false, false, false, 0};
constexpr open_mode append = {false, false, false, true, false, 0};
constexpr open_mode truncate = {false, false, false, false, true, 0};
constexpr open_mode create(u16 mode) {
    return {false, false, true, false, false, mode};
}
} // namespace mode

struct file {
    int fd;

    explicit file(char const *path, open_mode mode);
    ~file();

    usize read_chunk(slice<char> buf);
    vector<char> read_all(allocator *alloc, usize chunk_size = 512);

    usize write_chunk(slice<char> buf);
    void write_all(slice<char> buf);

    void flush();
};

inline void destroy(file &file);

} // namespace vixen

inline vixen::open_mode operator|(vixen::open_mode const &a, vixen::open_mode const &b);

#include "io/file.inl"
