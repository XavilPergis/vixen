#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/string.hpp"
#include "vixen/util.hpp"

#include <cstring>

#define _VIXEN_UTF8_CODEPOINT_VALID(codepoint)                          \
    VIXEN_DEBUG_ASSERT(::vixen::utf8::is_valid_for_encoding(codepoint), \
        "Codepoint U+{:X} is a not valid for UTF-8 encoding.",          \
        codepoint)

namespace vixen {

#pragma region "String"
// +------------------------------------------------------------------------------+
// | String                                                                       |
// +------------------------------------------------------------------------------+

#pragma region "String Initialization and Deinitialization"
// + ----- String Initialization and Deinitialization --------------------------- +

inline string::string(allocator *alloc) : data(vector<char>(alloc)) {}

inline string::string(allocator *alloc, const char *cstr) : data(vector<char>(alloc)) {
    push({cstr});
}

inline string::string(allocator *alloc, string_slice slice)
    : data(vector<char>(alloc, slice.len())) {
    push(slice);
}

inline string::string(allocator *alloc, usize default_capacity)
    : data(vector<char>(alloc, default_capacity)) {}

inline string::string(vector<char> &&data) : data(MOVE(data)) {
    // VIXEN_ASSERT(utf8::is_valid(data), "String data was not valid UTF-8.")
}

#pragma endregion
#pragma region "String Mutation"
// + ----- String Mutation ------------------------------------------------------ +

inline void string::push(u32 codepoint) {
    _VIXEN_UTF8_CODEPOINT_VALID(codepoint);
    utf8::encode(codepoint, data.reserve(utf8::encoded_len(codepoint)));
}

inline void string::push(string_slice slice) {
    util::copy_nonoverlapping(slice.begin(), data.reserve(slice.len()), slice.len());
}

inline void string::clear() {
    data.clear();
}

#pragma endregion
#pragma region "String Algorithms"
// + ----- String Algorithms ---------------------------------------------------- +

inline string string::clone(allocator *alloc) const {
    string buf(alloc, data.len());
    buf.push(slice<const char>(data));
    return buf;
}

// TODO: code duplication with the methods on `string_slice`. I wish there way a way to
// automatically convert types when using `.` or `->`... Like Rust's Deref trait.
inline option<usize> string::index_of(string_slice needle) const {
    return as_slice().index_of(needle);
}

inline option<usize> string::last_index_of(string_slice needle) const {
    return as_slice().last_index_of(needle);
}

inline option<usize> string::index_of(u32 needle) const {
    return as_slice().index_of(needle);
}

inline option<usize> string::last_index_of(u32 needle) const {
    return as_slice().last_index_of(needle);
}

inline bool string::starts_with(string_slice slice) const {
    return as_slice().starts_with(slice);
}

inline bool string::starts_with(u32 ch) const {
    return as_slice().starts_with(ch);
}

inline bool string::operator==(string_slice rhs) const {
    return as_slice() == rhs;
}

#pragma endregion
#pragma region "String Accessors"
// + ----- String Accessors ----------------------------------------------------- +

inline string_slice string::operator[](range range) const {
    return as_slice()[range];
}

inline string_slice string::operator[](range_from range) const {
    return as_slice()[range];
}

inline string_slice string::operator[](range_to range) const {
    return as_slice()[range];
}

inline char *string::begin() {
    return data.begin();
}

inline char *string::end() {
    return data.end();
}

inline const char *string::begin() const {
    return data.begin();
}

inline const char *string::end() const {
    return data.end();
}

inline usize string::len() const {
    return data.len();
}

#pragma endregion
#pragma region "String Conversion"
// + ----- String Conversion ---------------------------------------------------- +

inline string_slice string::as_slice() const {
    return slice<const char>({data.begin(), data.len()});
}

inline string::operator slice<const char>() const {
    return data;
}

inline string::operator slice<char>() {
    return data;
}

#pragma endregion
#pragma region "String Misc"
// + ----- String Misc ---------------------------------------------------------- +

template <typename S>
S &operator<<(S &stream, const string &str) {
    for (char ch : str.data) {
        stream << ch;
    }
    return stream;
}

namespace detail {
inline usize count_invalid_nuls(slice<char> data) {
    if (data.len == 0) {
        return 0;
    }

    usize nuls = 0;
    for (usize i = 0; i < data.len - 1; ++i) {
        if (data[i] == 0) {
            ++nuls;
        }
    }
    return nuls;
}
} // namespace detail

inline void null_terminate(vector<char> *data) {
    usize invalid_nuls = detail::count_invalid_nuls(*data);
    VIXEN_ASSERT(invalid_nuls == 0,
        "Tried to create a nul-terminated string out of a buffer with {} internal nuls.",
        invalid_nuls);

    if (data->as_const().last() != 0) {
        data->push(0);
    }
}

inline vector<char> to_null_terminated(allocator *alloc, string_slice str) {
    vector<char> out(alloc, str.len() + 1);
    util::copy_nonoverlapping(str.begin(), out.reserve(str.len()), str.len());

    null_terminate(&out);
    return out;
}

#pragma endregion

#pragma endregion
#pragma region "string_slice"
// +------------------------------------------------------------------------------+
// | string_slice                                                                  |
// +------------------------------------------------------------------------------+

#pragma region "string_slice Constructors"
// + ----- string_slice Constructors --------------------------------------------- +

inline string_slice::string_slice() : raw({}) {}
inline string_slice::string_slice(const char *cstr) : raw({cstr, std::strlen(cstr)}) {}
inline string_slice::string_slice(slice<const char> slice) : raw(slice) {}
inline string_slice::string_slice(string const &str) : raw({str.begin(), str.len()}) {}

#pragma endregion
#pragma region "string_slice Algorithms"
// + ----- string_slice Algorithms ----------------------------------------------- +

inline option<usize> string_slice::index_of(u32 codepoint) const {
    _VIXEN_UTF8_CODEPOINT_VALID(codepoint);

    char buf[4];
    return index_of(utf8::encode_slice(codepoint, buf));
}

inline option<usize> string_slice::last_index_of(u32 codepoint) const {
    _VIXEN_UTF8_CODEPOINT_VALID(codepoint);

    char buf[4];
    return last_index_of(utf8::encode_slice(codepoint, buf));
}

inline option<usize> string_slice::index_of(string_slice needle) const {
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

inline option<usize> string_slice::last_index_of(string_slice needle) const {
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

inline bool string_slice::starts_with(u32 codepoint) const {
    char buf[4];
    return starts_with(utf8::encode_slice(codepoint, buf));
}

inline bool string_slice::starts_with(string_slice needle) const {
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

inline vector<string> string_slice::split(allocator *alloc, string_slice token) const {
    vector<string> vec(alloc);
    split_to(alloc, token, vec);
    return vec;
}

inline void string_slice::split_to(
    allocator *alloc, string_slice token, vector<string> &out) const {
    string current_run(alloc);

    usize i = 0;
    while (i < len()) {
        if (raw[range(i, i + token.len())] == token.raw) {
            out.push(MOVE(current_run));
            current_run = string(alloc);
            i += token.len();
            continue;
        }

        current_run.push(raw[i]);
        ++i;
    }
}

#pragma endregion
#pragma region "string_slice Accessors"
// + ----- string_slice Accessors ------------------------------------------------ +

inline string_slice string_slice::operator[](range r) const {
    VIXEN_DEBUG_ASSERT(utf8::is_char_boundary(raw[r.start]),
        "Tried to slice string, but start index {} was not on a char boundary.",
        r.start);
    return string_slice(raw[r]);
}

inline string_slice string_slice::operator[](range_from range) const {
    VIXEN_DEBUG_ASSERT(utf8::is_char_boundary(raw[range.start]),
        "Tried to slice string, but start index {} was not on a char boundary.",
        range.start);
    return string_slice(raw[range]);
}

inline string_slice string_slice::operator[](range_to range) const {
    return string_slice(raw[range]);
}

inline const char *string_slice::begin() const {
    return raw.begin();
}

inline const char *string_slice::end() const {
    return raw.end();
}

inline usize string_slice::len() const {
    return raw.len;
}

inline bool string_slice::operator==(string_slice rhs) const {
    return raw == rhs.raw;
}

#pragma endregion
#pragma region "string_slice Misc"

template <typename S>
inline S &operator<<(S &stream, const string_slice &str) {
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

inline void destroy(string &string) {
    destroy(string.data);
}

template <typename H>
inline void hash(const string &value, H &hasher) {
    hash(value.data, hasher);
}

#pragma endregion

#pragma region "UTF-8"

namespace utf8 {

inline bool is_char_boundary(char utf8_char) {
    return (utf8_char & 0xc0) != 0xe0;
}

inline string_slice encode_slice(u32 codepoint, char *buf) {
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
