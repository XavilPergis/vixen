#pragma once

#include "vixen/allocator/allocator.hpp"
// #include "vixen/stream.hpp"
#include "vixen/string.hpp"
#include "vixen/util.hpp"

#include <cstring>

#define _VIXEN_UTF8_CODEPOINT_VALID(codepoint)                           \
    VIXEN_DEBUG_ASSERT_EXT(::vixen::utf8::isValidForEncoding(codepoint), \
        "Codepoint U+{:X} is a not valid for UTF-8 encoding.",           \
        codepoint)

namespace vixen {

#pragma region "String"
// +------------------------------------------------------------------------------+
// | String                                                                       |
// +------------------------------------------------------------------------------+

#pragma region "String Initialization and Deinitialization"
// + ----- String Initialization and Deinitialization --------------------------- +

inline String::String(Allocator &alloc) noexcept : mData(alloc) {}
inline String::String(Allocator &alloc, usize defaultCapacity) : mData(alloc, defaultCapacity) {}

inline String String::empty(Allocator &alloc) noexcept {
    return String(alloc);
}

inline String String::withCapacity(Allocator &alloc, usize defaultCapacity) {
    return String(alloc, defaultCapacity);
}

inline String String::copyFromAsciiNullTerminated(Allocator &alloc, const char *cstr) {
    Vector<char> data(alloc);
    while (*cstr != 0) {
        data.insertLast(*cstr);
        cstr += 1;
    }
    data.insertLast(char(0));
    return String(mv(data));
}

inline String String::copyFrom(Allocator &alloc, StringView slice) {
    Vector<char> data(alloc, slice.len() + 1);

    char *reserved = data.reserveLast(slice.len());
    util::arrayCopyNonoverlapping(slice.begin(), reserved, slice.len());
    data.unsafeIncreaseLengthBy(slice.len());
    data.insertLast(char(0));

    return String(mv(data));
}

#pragma endregion
#pragma region "String Mutation"
// + ----- String Mutation ------------------------------------------------------ +

inline void String::insertLast(u32 codepoint) {
    _VIXEN_UTF8_CODEPOINT_VALID(codepoint);
    auto encodedLen = utf8::encodedLength(codepoint);
    utf8::encode(codepoint, mData.reserveLast(encodedLen));
    mData.unsafeIncreaseLengthBy(encodedLen);
}

inline void String::insertLast(StringView slice) {
    util::arrayCopyNonoverlapping(slice.begin(), mData.reserveLast(slice.len()), slice.len());
    mData.unsafeIncreaseLengthBy(slice.len());
}

inline void String::removeAll() noexcept {
    mData.removeAll();
}

#pragma endregion
#pragma region "String Algorithms"
// + ----- String Algorithms ---------------------------------------------------- +

// TODO: code duplication with the methods on `StringView`. I wish there way a way to
// automatically convert types when using `.` or `->`... Like Rust's Deref trait.
inline Option<usize> String::indexOf(StringView needle) const noexcept {
    return asSlice().indexOf(needle);
}

inline Option<usize> String::lastIndexOf(StringView needle) const noexcept {
    return asSlice().lastIndexOf(needle);
}

inline Option<usize> String::indexOf(u32 needle) const noexcept {
    return asSlice().indexOf(needle);
}

inline Option<usize> String::lastIndexOf(u32 needle) const noexcept {
    return asSlice().lastIndexOf(needle);
}

inline bool String::startsWith(StringView slice) const noexcept {
    return asSlice().startsWith(slice);
}

inline bool String::startsWith(u32 ch) const noexcept {
    return asSlice().startsWith(ch);
}

inline bool String::operator==(StringView rhs) const noexcept {
    return asSlice() == rhs;
}

#pragma endregion
#pragma region "String Accessors"
// + ----- String Accessors ----------------------------------------------------- +

inline StringView String::operator[](Range range) const noexcept {
    return asSlice()[range];
}

inline StringView String::operator[](RangeFrom range) const noexcept {
    return asSlice()[range];
}

inline StringView String::operator[](RangeTo range) const noexcept {
    return asSlice()[range];
}

inline char *String::begin() noexcept {
    return mData.begin();
}

inline char *String::end() noexcept {
    return mData.end();
}

inline const char *String::begin() const noexcept {
    return mData.begin();
}

inline const char *String::end() const noexcept {
    return mData.end();
}

inline usize String::len() const noexcept {
    return mData.len();
}

#pragma endregion
#pragma region "String Conversion"
// + ----- String Conversion ---------------------------------------------------- +

inline StringView String::asSlice() const noexcept {
    return View<const char>(mData.begin(), mData.len());
}

inline String::operator View<const char>() const noexcept {
    return mData;
}

inline String::operator View<char>() noexcept {
    return mData;
}

#pragma endregion
#pragma region "String Misc"

template <typename S>
S &operator<<(S &stream, const String &str) {
    for (char ch : str.mData) {
        stream << ch;
    }
    return stream;
}

namespace detail {
inline usize count_invalid_nuls(View<char> data) {
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
        data.insertLast(char(0));
    }
}

inline Vector<char> to_null_terminated(Allocator &alloc, StringView str) {
    Vector<char> out(alloc, str.len() + 1);
    util::arrayCopyNonoverlapping(str.begin(), out.reserveLast(str.len()), str.len());
    out.unsafeIncreaseLengthBy(str.len());

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

inline StringView::StringView() noexcept : raw({}) {}
inline StringView::StringView(const char *cstr) : raw({cstr, std::strlen(cstr)}) {}
inline StringView::StringView(const char *str, usize len) : raw({str, len}) {}
inline StringView::StringView(View<const char> slice) : raw(slice) {}
inline StringView::StringView(String const &str) noexcept : raw({str.begin(), str.len()}) {}

inline StringView StringView::empty() noexcept {
    return StringView{};
}

inline StringView StringView::fromUtf8(View<const char> const &data) {
    return StringView{data};
}

inline StringView StringView::fromAsciiNullTerminated(char const *cstr) {
    return StringView{cstr};
}

#pragma endregion
#pragma region "string_slice Algorithms"
// + ----- StringView Algorithms ----------------------------------------------- +

inline Option<usize> StringView::indexOf(u32 codepoint) const noexcept {
    _VIXEN_UTF8_CODEPOINT_VALID(codepoint);

    char buf[4];
    return indexOf(utf8::encodeView(codepoint, buf));
}

inline Option<usize> StringView::lastIndexOf(u32 codepoint) const noexcept {
    _VIXEN_UTF8_CODEPOINT_VALID(codepoint);

    char buf[4];
    return lastIndexOf(utf8::encodeView(codepoint, buf));
}

inline Option<usize> StringView::indexOf(StringView needle) const noexcept {
    // Can't find something that's bigger than the haystack!
    if (needle.len() > len()) {
        return {};
    }

    // We only need to search the positions where we know `needle` can fit.
    // From the previous check, we know the subtraction can't underflow.
    usize searchLen = len() - needle.len();
    for (usize i = 0; i < searchLen; ++i) {
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

inline Option<usize> StringView::lastIndexOf(StringView needle) const noexcept {
    if (needle.len() > len()) {
        return {};
    }

    usize searchLen = len() - needle.len();
    for (usize i = 0; i < searchLen; ++i) {
        for (usize j = 0; j < needle.len(); ++j) {
            if (raw[searchLen - i + j] != needle.raw[j]) {
                goto nomatch;
            }
        }
        return searchLen - i;
nomatch:;
    }

    return {};
}

inline bool StringView::startsWith(u32 codepoint) const noexcept {
    char buf[4];
    return startsWith(utf8::encodeView(codepoint, buf));
}

inline bool StringView::startsWith(StringView needle) const noexcept {
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

inline Vector<String> StringView::split(Allocator &alloc, StringView token) const {
    Vector<String> vec(alloc);
    splitTo(alloc, token, vec);
    return vec;
}

inline void StringView::splitTo(Allocator &alloc, StringView token, Vector<String> &out) const {
    String current_run(alloc);

    usize i = 0;
    while (i < len()) {
        if (raw[range(i, i + token.len())] == token.raw) {
            out.insertLast(mv(current_run));
            current_run = String(alloc);
            i += token.len();
            continue;
        }

        current_run.insertLast(raw[i]);
        ++i;
    }
}

#pragma endregion
#pragma region "string_slice Accessors"
// + ----- StringView Accessors ------------------------------------------------ +

inline StringView StringView::operator[](Range r) const noexcept {
    VIXEN_DEBUG_ASSERT_EXT(utf8::isCharBoundary(raw[r.start]),
        "Tried to slice String, but start index {} was not on a char boundary.",
        r.start);
    return StringView(raw[r]);
}

inline StringView StringView::operator[](RangeFrom range) const noexcept {
    VIXEN_DEBUG_ASSERT_EXT(utf8::isCharBoundary(raw[range.start]),
        "Tried to slice String, but start index {} was not on a char boundary.",
        range.start);
    return StringView(raw[range]);
}

inline StringView StringView::operator[](RangeTo range) const noexcept {
    return StringView(raw[range]);
}

inline const char *StringView::begin() const noexcept {
    return raw.begin();
}

inline const char *StringView::end() const noexcept {
    return raw.end();
}

inline usize StringView::len() const noexcept {
    return raw.len();
}

inline bool StringView::operator==(StringView rhs) const noexcept {
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
inline void hash(const String &value, H &hasher) noexcept {
    hash(value.mData, hasher);
}

template <typename H>
void hash(StringView const &value, H &hasher) noexcept {
    hash(value.raw, hasher);
}

#pragma endregion

#pragma region "UTF-8"

namespace utf8 {

inline bool isCharBoundary(char utf8Char) noexcept {
    return (utf8Char & 0xc0) != 0x80;
}

inline StringView encodeView(u32 codepoint, char *buf) noexcept {
    _VIXEN_UTF8_CODEPOINT_VALID(codepoint);

    encode(codepoint, buf);
    return StringView::fromUtf8({buf, encodedLength(codepoint)});
}

inline usize encodedLength(u32 codepoint) noexcept {
    _VIXEN_UTF8_CODEPOINT_VALID(codepoint);

    // clang-format off
    if      (codepoint <= 0x7f)     { return 1; }
    else if (codepoint <= 0x7ff)    { return 2; }
    else if (codepoint <= 0xffff)   { return 3; }
    else if (codepoint <= 0x10ffff) { return 4; }
    // clang-format on

    VIXEN_UNREACHABLE();
}

inline bool isValidForEncoding(u32 codepoint) noexcept {
    const bool inRange = codepoint <= 0x10FFFF;
    const bool utf16Reserved = codepoint >= 0xD800 && codepoint <= 0xDFFF;
    return inRange && !utf16Reserved;
}

} // namespace utf8

#pragma endregion

} // namespace vixen
