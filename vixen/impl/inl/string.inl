#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/stream.hpp"
#include "vixen/string.hpp"
#include "vixen/util.hpp"

#include <cstring>

#define _VIXEN_UTF8_CODEPOINT_VALID(codepoint)                              \
    VIXEN_DEBUG_ASSERT_EXT(::vixen::utf8::is_valid_for_encoding(codepoint), \
        "Codepoint U+{:X} is a not valid for UTF-8 encoding.",              \
        codepoint)

namespace vixen {

#pragma region "String"
// +------------------------------------------------------------------------------+
// | String                                                                       |
// +------------------------------------------------------------------------------+

#pragma region "String Initialization and Deinitialization"
// + ----- String Initialization and Deinitialization --------------------------- +

inline String::String(Allocator *alloc) : data(Vector<char>(alloc)) {}

inline String::String(Allocator *alloc, const char *cstr) : data(Vector<char>(alloc)) {
    push({cstr});
}

inline String::String(Allocator *alloc, StringView slice) : data(Vector<char>(alloc, slice.len())) {
    push(slice);
}

inline String::String(Allocator *alloc, usize default_capacity)
    : data(Vector<char>(alloc, default_capacity)) {}

inline String::String(Vector<char> &&data) : data(mv(data)) {
    // VIXEN_ASSERT_EXT(utf8::is_valid(data), "String data was not valid UTF-8.")
}

#pragma endregion
#pragma region "String Mutation"
// + ----- String Mutation ------------------------------------------------------ +

inline void String::push(u32 codepoint) {
    _VIXEN_UTF8_CODEPOINT_VALID(codepoint);
    utf8::encode(codepoint, data.reserve(utf8::encoded_len(codepoint)));
}

inline void String::push(StringView slice) {
    util::copy_nonoverlapping(slice.begin(), data.reserve(slice.len()), slice.len());
}

inline void String::clear() {
    data.clear();
}

#pragma endregion
#pragma region "String Algorithms"
// + ----- String Algorithms ---------------------------------------------------- +

// TODO: code duplication with the methods on `StringView`. I wish there way a way to
// automatically convert types when using `.` or `->`... Like Rust's Deref trait.
inline Option<usize> String::index_of(StringView needle) const {
    return as_slice().index_of(needle);
}

inline Option<usize> String::last_index_of(StringView needle) const {
    return as_slice().last_index_of(needle);
}

inline Option<usize> String::index_of(u32 needle) const {
    return as_slice().index_of(needle);
}

inline Option<usize> String::last_index_of(u32 needle) const {
    return as_slice().last_index_of(needle);
}

inline bool String::starts_with(StringView slice) const {
    return as_slice().starts_with(slice);
}

inline bool String::starts_with(u32 ch) const {
    return as_slice().starts_with(ch);
}

inline bool String::operator==(StringView rhs) const {
    return as_slice() == rhs;
}

#pragma endregion
#pragma region "String Accessors"
// + ----- String Accessors ----------------------------------------------------- +

inline StringView String::operator[](Range range) const {
    return as_slice()[range];
}

inline StringView String::operator[](RangeFrom range) const {
    return as_slice()[range];
}

inline StringView String::operator[](RangeTo range) const {
    return as_slice()[range];
}

inline char *String::begin() {
    return data.begin();
}

inline char *String::end() {
    return data.end();
}

inline const char *String::begin() const {
    return data.begin();
}

inline const char *String::end() const {
    return data.end();
}

inline usize String::len() const {
    return data.len();
}

#pragma endregion
#pragma region "String Conversion"
// + ----- String Conversion ---------------------------------------------------- +

inline StringView String::as_slice() const {
    return Slice<const char>(data.begin(), data.len());
}

inline String::operator Slice<const char>() const {
    return data;
}

inline String::operator Slice<char>() {
    return data;
}

#pragma endregion
#pragma region "String Misc"

template <typename S>
S &operator<<(S &stream, const String &str) {
    for (char ch : str.data) {
        stream << ch;
    }
    return stream;
}

namespace detail {
inline usize count_invalid_nuls(Slice<char> data) {
    if (data.len() == 0) {
        return 0;
    }

    usize nuls = 0;
    for (usize i = 0; i < data.len() - 1; ++i) {
        if (data[i] == 0) {
            ++nuls;
        }
    }
    return nuls;
}
} // namespace detail

inline void null_terminate(Vector<char> &data) {
    usize invalid_nuls = detail::count_invalid_nuls(data);
    VIXEN_ASSERT_EXT(invalid_nuls == 0,
        "Tried to create a nul-terminated String out of a buffer with {} internal nuls.",
        invalid_nuls);

    if (const_cast<Vector<char> const &>(data).last() != 0) {
        data.push(0);
    }
}

inline Vector<char> to_null_terminated(Allocator *alloc, StringView str) {
    Vector<char> out(alloc, str.len() + 1);
    util::copy_nonoverlapping(str.begin(), out.reserve(str.len()), str.len());

    null_terminate(out);
    return out;
}

#pragma endregion

#pragma endregion
#pragma region "string_slice"
// +-------------------------------------------------------------------------------+
// | StringView                                                                  |
// +-------------------------------------------------------------------------------+

#pragma region "string_slice Constructors"
// + ----- StringView Constructors --------------------------------------------- +

inline StringView::StringView() : raw({}) {}
inline StringView::StringView(const char *cstr) : raw({cstr, std::strlen(cstr)}) {}
inline StringView::StringView(const char *str, usize len) : raw({str, len}) {}
inline StringView::StringView(Slice<const char> slice) : raw(slice) {}
inline StringView::StringView(String const &str) : raw({str.begin(), str.len()}) {}

#pragma endregion
#pragma region "string_slice Algorithms"
// + ----- StringView Algorithms ----------------------------------------------- +

inline Option<usize> StringView::index_of(u32 codepoint) const {
    _VIXEN_UTF8_CODEPOINT_VALID(codepoint);

    char buf[4];
    return index_of(utf8::encode_slice(codepoint, buf));
}

inline Option<usize> StringView::last_index_of(u32 codepoint) const {
    _VIXEN_UTF8_CODEPOINT_VALID(codepoint);

    char buf[4];
    return last_index_of(utf8::encode_slice(codepoint, buf));
}

inline Option<usize> StringView::index_of(StringView needle) const {
    // Can't find something that's bigger than the haystack!
    if (needle.len() > len()) {
        return {};
    }

    // We only need to search the positions where we know `needle` can fit.
    // From the previous check, we know the subtraction can't underflow.
    usize search_len = len() - needle.len();
    for (usize i = 0; i < search_len; ++i) {
        for (usize j = 0; j < needle.len(); ++j) {
            // Bail early if we find a mismatch, so we don't waste time searching data we know isn't
            // a match.
            if (raw[i + j] != needle.raw[j]) {
                goto nomatch;
            }
        }

        // Getting through the inner loop without jumping means the entire sequence matched
        return i;
nomatch:;
    }

    return {};
}

inline Option<usize> StringView::last_index_of(StringView needle) const {
    if (needle.len() > len()) {
        return {};
    }

    usize search_len = len() - needle.len();
    for (usize i = 0; i < search_len; ++i) {
        for (usize j = 0; j < needle.len(); ++j) {
            if (raw[search_len - i + j] != needle.raw[j]) {
                goto nomatch;
            }
        }
        return search_len - i;
nomatch:;
    }

    return {};
}

inline bool StringView::starts_with(u32 codepoint) const {
    char buf[4];
    return starts_with(utf8::encode_slice(codepoint, buf));
}

inline bool StringView::starts_with(StringView needle) const {
    if (needle.len() > len()) {
        return false;
    }
    for (usize i = 0; i < needle.len(); ++i) {
        if (needle.raw[i] != raw[i]) {
            return false;
        }
    }
    return true;
}

inline Vector<String> StringView::split(Allocator *alloc, StringView token) const {
    Vector<String> vec(alloc);
    split_to(alloc, token, vec);
    return vec;
}

inline void StringView::split_to(Allocator *alloc, StringView token, Vector<String> &out) const {
    String current_run(alloc);

    usize i = 0;
    while (i < len()) {
        if (raw[range(i, i + token.len())] == token.raw) {
            out.push(mv(current_run));
            current_run = String(alloc);
            i += token.len();
            continue;
        }

        current_run.push(raw[i]);
        ++i;
    }
}

#pragma endregion
#pragma region "string_slice Accessors"
// + ----- StringView Accessors ------------------------------------------------ +

inline StringView StringView::operator[](Range r) const {
    VIXEN_DEBUG_ASSERT_EXT(utf8::is_char_boundary(raw[r.start]),
        "Tried to slice String, but start index {} was not on a char boundary.",
        r.start);
    return StringView(raw[r]);
}

inline StringView StringView::operator[](RangeFrom range) const {
    VIXEN_DEBUG_ASSERT_EXT(utf8::is_char_boundary(raw[range.start]),
        "Tried to slice String, but start index {} was not on a char boundary.",
        range.start);
    return StringView(raw[range]);
}

inline StringView StringView::operator[](RangeTo range) const {
    return StringView(raw[range]);
}

inline const char *StringView::begin() const {
    return raw.begin();
}

inline const char *StringView::end() const {
    return raw.end();
}

inline usize StringView::len() const {
    return raw.len();
}

inline bool StringView::operator==(StringView rhs) const {
    return raw == rhs.raw;
}

#pragma endregion
#pragma region "string_slice Misc"

template <typename S>
inline S &operator<<(S &stream, const StringView &str) {
    for (char ch : str.raw) {
        stream << ch;
    }
    return stream;
}

#pragma endregion

#pragma endregion
#pragma region "Traits"
// +------------------------------------------------------------------------------+
// | Traits                                                                       |
// +------------------------------------------------------------------------------+

template <typename H>
inline void hash(const String &value, H &hasher) {
    hash(value.data, hasher);
}

#pragma endregion

#pragma region "UTF-8"

namespace utf8 {

inline bool is_char_boundary(char utf8_char) {
    return (utf8_char & 0xc0) != 0x80;
}

inline StringView encode_slice(u32 codepoint, char *buf) {
    _VIXEN_UTF8_CODEPOINT_VALID(codepoint);

    encode(codepoint, buf);
    return {{buf, encoded_len(codepoint)}};
}

inline usize encoded_len(u32 codepoint) {
    _VIXEN_UTF8_CODEPOINT_VALID(codepoint);

    // clang-format off
    if      (codepoint <= 0x7f)     { return 1; }
    else if (codepoint <= 0x7ff)    { return 2; }
    else if (codepoint <= 0xffff)   { return 3; }
    else if (codepoint <= 0x10ffff) { return 4; }
    // clang-format on

    VIXEN_UNREACHABLE();
}

inline bool is_valid_for_encoding(u32 codepoint) {
    const bool in_range = codepoint <= 0x10FFFF;
    const bool utf16_reserved = codepoint >= 0xD800 && codepoint <= 0xDFFF;
    return in_range && !utf16_reserved;
}

} // namespace utf8

#pragma endregion

} // namespace vixen
