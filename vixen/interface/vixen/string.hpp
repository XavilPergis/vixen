#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/option.hpp"
#include "vixen/slice.hpp"
#include "vixen/traits.hpp"
#include "vixen/vec.hpp"

#include <spdlog/fmt/ostr.h>

namespace vixen {

struct string_slice;

struct string {
    vector<char> data;

    string() = default;

    explicit string(allocator *alloc);
    string(allocator *alloc, const char *cstr);
    string(allocator *alloc, string_slice slice);
    string(allocator *alloc, usize default_capacity);
    explicit string(vector<char> &&data);

    /// Encodes the Unicode codepoint `codepoint` into UTF-8 and appends the encoded bytes onto the
    /// end of the string. `codepoint` MUST be a valid unicode codepoint, as
    /// `utf8::is_valid_for_encoding` can determine.
    void push(u32 codepoint);

    void push(string_slice slice);

    void clear();

    /// Creates a new distinct String with the same contents as the current one. It is the caller's
    /// responsibility to free the returned string.
    string clone(allocator *alloc) const;

    option<usize> index_of(string_slice needle) const;
    option<usize> index_of(u32 needle) const;
    option<usize> last_index_of(string_slice needle) const;
    option<usize> last_index_of(u32 needle) const;

    bool starts_with(string_slice slice) const;
    bool starts_with(u32 ch) const;

    bool operator==(string_slice rhs) const;

    string_slice operator[](range range) const;
    string_slice operator[](range_from range) const;
    string_slice operator[](range_to range) const;

    char *begin();
    char *end();
    const char *begin() const;
    const char *end() const;
    usize len() const;

    string_slice as_slice() const;
    operator slice<const char>() const;
    operator slice<char>();

    template <typename S>
    friend S &operator<<(S &stream, const string &str);
};

struct string_slice {
    slice<const char> raw;

    string_slice();
    string_slice(const char *cstr);
    string_slice(slice<const char> slice);
    string_slice(string const &str);

    option<usize> index_of(string_slice needle) const;
    option<usize> index_of(u32 needle) const;
    option<usize> last_index_of(string_slice needle) const;
    option<usize> last_index_of(u32 needle) const;

    bool starts_with(string_slice slice) const;
    bool starts_with(u32 codepoint) const;

    void split_to(allocator *alloc, string_slice token, vector<string> &out) const;

    /// Note that both the vector and the strings within the vector are allocated using `alloc`.
    vector<string> split(allocator *alloc, string_slice token) const;

    string to_string(allocator *alloc) const;

    operator slice<const char>() const {
        return raw;
    }

    string_slice operator[](range range) const;
    string_slice operator[](range_from range) const;
    string_slice operator[](range_to range) const;
    const char *begin() const;
    const char *end() const;
    usize len() const;

    bool operator==(string_slice rhs) const;

    template <typename S>
    friend S &operator<<(S &stream, const string_slice &str);
};

template <typename H>
void hash(string const &value, H &hasher);

template <typename H>
void hash(string_slice const &value, H &hasher);

inline void null_terminate(vector<char> *data);
inline vector<char> to_null_terminated(allocator *alloc, string_slice str);

namespace utf8 {

inline bool is_char_boundary(char utf8_char);

/// Returns whether `codepoint` can be safely encoded into UTF-8.
inline bool is_valid_for_encoding(u32 codepoint);

/// Returns how many bytes are needed to store the UTF-8 encoding for `codepoint`.
inline usize encoded_len(u32 codepoint);

/// Writes the UTF-8 encoding of `codepoint` into the buffer `buf`. `codepoint` MUST be a valid
/// codepoint (as `is_valid_for_encoding` can determine), and `buf` MUST be at least the number of
/// bytes as the encoding of `codepoint` requires (as `utf8_encoded_len` can determine).
void encode(u32 codepoint, char *buf);

/// Like `encode`, but returns a slice of the encoded UTF-8 whose lifetime is the same as `buf`'s.
string_slice encode_slice(u32 codepoint, char *buf);

/// Decodes the UTF-8 sequence at the start of the slice, or returns nothing if the encoding is
/// wrong.
option<u32> decode(slice<const char> buf);

bool is_valid(slice<const char> buf);

} // namespace utf8

} // namespace vixen

inline ::vixen::string_slice operator"" _s(const char *cstr, usize len) {
    return {{cstr, len}};
}

#include "string.inl"
