#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/option.hpp"
#include "vixen/slice.hpp"
#include "vixen/traits.hpp"
#include "vixen/vec.hpp"

#include <spdlog/fmt/ostr.h>

namespace vixen {

struct StringView;

/// @ingroup vixen_data_structures
/// @brief UTF-8 String.
struct String {
    Vector<char> data;

    String() = default;
    String(String const &) = delete;
    String &operator=(String const &) = delete;
    String(String &&) = default;
    String &operator=(String &&) = default;

    String(copy_tag_t, Allocator *alloc, String const &other) : data(data.clone(alloc)) {}

    explicit String(Allocator *alloc);
    String(Allocator *alloc, const char *cstr);
    String(Allocator *alloc, StringView slice);
    String(Allocator *alloc, usize default_capacity);
    explicit String(Vector<char> &&data);

    /// Encodes the Unicode codepoint `codepoint` into UTF-8 and appends the encoded bytes onto the
    /// end of the String. `codepoint` MUST be a valid unicode codepoint, as
    /// `utf8::is_valid_for_encoding` can determine.
    void push(u32 codepoint);

    void push(StringView slice);

    void clear();

    VIXEN_DEFINE_CLONE_METHOD(String)

    Option<usize> index_of(StringView needle) const;
    Option<usize> index_of(u32 needle) const;
    Option<usize> last_index_of(StringView needle) const;
    Option<usize> last_index_of(u32 needle) const;

    bool starts_with(StringView slice) const;
    bool starts_with(u32 ch) const;

    bool operator==(StringView rhs) const;

    StringView operator[](Range range) const;
    StringView operator[](RangeFrom range) const;
    StringView operator[](RangeTo range) const;

    char *begin();
    char *end();
    const char *begin() const;
    const char *end() const;
    usize len() const;

    StringView as_slice() const;
    operator Slice<const char>() const;
    operator Slice<char>();

    template <typename S>
    friend S &operator<<(S &stream, const String &str);
};

struct StringView {
    Slice<const char> raw;

    StringView();
    StringView(const char *cstr);
    StringView(const char *str, usize len);
    StringView(Slice<const char> slice);
    StringView(String const &str);

    Option<usize> index_of(StringView needle) const;
    Option<usize> index_of(u32 needle) const;
    Option<usize> last_index_of(StringView needle) const;
    Option<usize> last_index_of(u32 needle) const;

    bool starts_with(StringView slice) const;
    bool starts_with(u32 codepoint) const;

    void split_to(Allocator *alloc, StringView token, Vector<String> &out) const;

    /// Note that both the vector and the strings within the vector are allocated using `alloc`.
    Vector<String> split(Allocator *alloc, StringView token) const;

    String to_string(Allocator *alloc) const;

    operator Slice<const char>() const {
        return raw;
    }

    StringView operator[](Range range) const;
    StringView operator[](RangeFrom range) const;
    StringView operator[](RangeTo range) const;
    const char *begin() const;
    const char *end() const;
    usize len() const;

    bool operator==(StringView rhs) const;

    template <typename S>
    friend S &operator<<(S &stream, const StringView &str);
};

template <typename H>
void hash(String const &value, H &hasher);

template <typename H>
void hash(StringView const &value, H &hasher);

inline void null_terminate(Vector<char> &data);
inline Vector<char> to_null_terminated(Allocator *alloc, StringView str);

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
StringView encode_slice(u32 codepoint, char *buf);

/// Decodes the UTF-8 sequence at the start of the slice, or returns nothing if the encoding is
/// wrong.
Option<u32> decode(Slice<const char> buf);

bool is_valid(Slice<const char> buf);

} // namespace utf8

} // namespace vixen

inline ::vixen::StringView operator"" _s(const char *cstr, usize len) {
    return {{cstr, len}};
}

#include "string.inl"

// String foo = "1234";
// alloc(foo, myvar) = "1234";