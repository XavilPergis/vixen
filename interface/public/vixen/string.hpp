#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/option.hpp"
#include "vixen/slice.hpp"
#include "vixen/traits.hpp"
#include "vixen/vector.hpp"

#include <iterator>

#include <spdlog/fmt/ostr.h>

namespace vixen {

struct StringView;
struct String;
struct StringOutputIterator;

/// @ingroup vixen_data_structures
/// @brief UTF-8 String.
struct String {
    String() = default;
    explicit String(Vector<char> &&stringDataUtf8) noexcept : mData(mv(stringDataUtf8)) {}
    VIXEN_MOVE_ONLY(String)

    // Allocator-aware copy ctor
    String(copy_tag_t, Allocator &alloc, String const &other) : mData(other.mData.clone(alloc)) {}
    VIXEN_DEFINE_CLONE_METHOD(String)

    explicit String(Allocator &alloc) noexcept;
    String(Allocator &alloc, usize defaultCapacity);

    static String empty(Allocator &alloc) noexcept;
    static String withCapacity(Allocator &alloc, usize defaultCapacity);

    static String fromUtf8(Vector<char> &&data);
    static String copyFromAsciiNullTerminated(Allocator &alloc, char const *cstr);
    static String copyFrom(Allocator &alloc, StringView slice);

    StringView asSlice() const noexcept;
    operator View<const char>() const noexcept;
    operator View<char>() noexcept;

    /**
     * @warning When modifying the underlying slice, the invariants of this string type must be
     * upheld. Most notably, it must remain a valid UTF-8 byte sequence.
     */
    View<char> charData() noexcept { return mData; }
    View<const char> charData() const noexcept { return mData; }

    Vector<char> intoCharData() noexcept { return mv(mData); }

    char *reserveLast(usize chars) { return mData.reserveLast(chars); }

    void unsafeGrowBy(usize elements) noexcept { mData.unsafeIncreaseLengthBy(elements); }
    void decreaseLengthBy(usize elements) noexcept { mData.decreaseLengthBy(elements); }
    void decreaseLengthTo(usize length) noexcept { mData.decreaseLengthTo(length); }
    void unsafeSetLengthTo(usize length) noexcept { mData.unsafeSetLengthTo(length); }

    void shrinkToFit(usize capacity) noexcept { mData.shrinkTo(capacity); }
    void shrinkToFit() noexcept { mData.shrinkToFit(); }

    /// Encodes the Unicode codepoint `codepoint` into UTF-8 and appends the encoded bytes onto the
    /// end of the String. `codepoint` MUST be a valid unicode codepoint, as
    /// `utf8::isValidForEncoding` can determine.
    void insertLast(u32 codepoint);
    void insertLast(StringView slice);

    void removeAll() noexcept;

    Option<usize> indexOf(StringView needle) const noexcept;
    Option<usize> indexOf(u32 needle) const noexcept;
    Option<usize> lastIndexOf(StringView needle) const noexcept;
    Option<usize> lastIndexOf(u32 needle) const noexcept;

    bool startsWith(StringView slice) const noexcept;
    bool startsWith(u32 ch) const noexcept;

    StringOutputIterator insertLastIterator() noexcept;

    bool operator==(StringView rhs) const noexcept;

    StringView operator[](Range range) const noexcept;
    StringView operator[](RangeFrom range) const noexcept;
    StringView operator[](RangeTo range) const noexcept;

    char *begin() noexcept;
    char *end() noexcept;
    const char *begin() const noexcept;
    const char *end() const noexcept;
    usize len() const noexcept;
    usize capacity() const noexcept { return mData.capacity(); }

    template <typename S>
    friend S &operator<<(S &stream, const String &str);

private:
    Vector<char> mData;
};

struct StringOutputIterator {
    explicit StringOutputIterator(String *output) : output(output) {}

    using iterator_category = std::output_iterator_tag;
    using value_type = void;
    using difference_type = void;
    using pointer = void;
    using reference = void;

    StringOutputIterator &operator=(char value) {
        output->insertLast(value);
        return *this;
    }

    // clang-format off
    StringOutputIterator &operator*() noexcept { return *this; }
    StringOutputIterator &operator++() noexcept { return *this; }
    StringOutputIterator &operator++(int) noexcept { return *this; }
    // clang-format on

private:
    String *output;
};

inline StringOutputIterator String::insertLastIterator() noexcept {
    return StringOutputIterator{this};
}

struct StringView {
    View<const char> raw;

    StringView() noexcept;

    // implicit conversions. is this a good idea? or should we force users to be more explicit?
    StringView(String const &str) noexcept;
    StringView(const char *cstr);
    StringView(const char *str, usize len);
    StringView(View<const char> slice);

    static StringView empty() noexcept;
    static StringView fromUtf8(View<const char> const &data);
    static StringView fromAsciiNullTerminated(char const *cstr);

    Option<usize> indexOf(StringView needle) const noexcept;
    Option<usize> indexOf(u32 needle) const noexcept;
    Option<usize> lastIndexOf(StringView needle) const noexcept;
    Option<usize> lastIndexOf(u32 needle) const noexcept;

    bool startsWith(StringView slice) const noexcept;
    bool startsWith(u32 codepoint) const noexcept;

    void splitTo(Allocator &alloc, StringView token, Vector<String> &out) const;

    /// Note that both the vector and the strings within the vector are allocated using `alloc`.
    Vector<String> split(Allocator &alloc, StringView token) const;

    String to_string(Allocator &alloc) const;

    operator View<const char>() const noexcept { return raw; }

    StringView operator[](Range range) const noexcept;
    StringView operator[](RangeFrom range) const noexcept;
    StringView operator[](RangeTo range) const noexcept;
    const char *begin() const noexcept;
    const char *end() const noexcept;
    usize len() const noexcept;

    bool operator==(StringView rhs) const noexcept;

    template <typename S>
    friend S &operator<<(S &stream, const StringView &str);
};

template <typename H>
void hash(String const &value, H &hasher) noexcept;

template <typename H>
void hash(StringView const &value, H &hasher) noexcept;

inline void null_terminate(Vector<char> &data);
inline Vector<char> to_null_terminated(Allocator &alloc, StringView str);

namespace utf8 {

inline bool isCharBoundary(char utf8Char) noexcept;

/// Returns whether `codepoint` can be safely encoded into UTF-8.
inline bool isValidForEncoding(u32 codepoint) noexcept;

/// Returns how many bytes are needed to store the UTF-8 encoding for `codepoint`.
inline usize encodedLength(u32 codepoint) noexcept;

/// Writes the UTF-8 encoding of `codepoint` into the buffer `buf`. `codepoint` MUST be a valid
/// codepoint (as `isValidForEncoding` can determine), and `buf` MUST be at least the number of
/// bytes as the encoding of `codepoint` requires (as `utf8_encoded_len` can determine).
void encode(u32 codepoint, char *buf) noexcept;

/// Like `encode`, but returns a slice of the encoded UTF-8 whose lifetime is the same as `buf`'s.
StringView encodeView(u32 codepoint, char *buf) noexcept;

/// Decodes the UTF-8 sequence at the start of the slice, or returns nothing if the encoding is
/// wrong.
Option<u32> decode(View<const char> buf) noexcept;

bool isEncodedTextValid(View<const char> buf) noexcept;

} // namespace utf8

} // namespace vixen

inline ::vixen::StringView operator"" _s(const char *cstr, usize len) {
    return {{cstr, len}};
}

#include "string.inl"

// String foo = "1234";
// alloc(foo, myvar) = "1234";