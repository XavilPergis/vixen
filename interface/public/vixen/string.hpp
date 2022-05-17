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
    explicit String(Vector<char> &&stringDataUtf8) : mData(mv(stringDataUtf8)) {}
    VIXEN_MOVE_ONLY(String)

    // Allocator-aware copy ctor
    String(copy_tag_t, Allocator &alloc, String const &other) : mData(other.mData.clone(alloc)) {}
    VIXEN_DEFINE_CLONE_METHOD(String)

    explicit String(Allocator &alloc);
    String(Allocator &alloc, usize defaultCapacity);

    static String empty(Allocator &alloc);
    static String withCapacity(Allocator &alloc, usize defaultCapacity);

    static String fromUtf8(Vector<char> &&data);
    static String copyFromAsciiNullTerminated(Allocator &alloc, char const *cstr);
    static String copyFrom(Allocator &alloc, StringView slice);

    StringView asSlice() const;
    operator View<const char>() const;
    operator View<char>();

    /**
     * @warning When modifying the underlying slice, the invariants of this string type must be
     * upheld. Most notably, it must remain a valid UTF-8 byte sequence.
     */
    View<char> charData() { return mData; }
    View<const char> charData() const { return mData; }

    Vector<char> intoCharData() { return mv(mData); }

    /// Encodes the Unicode codepoint `codepoint` into UTF-8 and appends the encoded bytes onto the
    /// end of the String. `codepoint` MUST be a valid unicode codepoint, as
    /// `utf8::is_valid_for_encoding` can determine.
    void push(u32 codepoint);
    void push(StringView slice);

    void clear();

    Option<usize> indexOf(StringView needle) const;
    Option<usize> indexOf(u32 needle) const;
    Option<usize> lastIndexOf(StringView needle) const;
    Option<usize> lastIndexOf(u32 needle) const;

    bool startsWith(StringView slice) const;
    bool startsWith(u32 ch) const;

    StringOutputIterator insertLastIterator();

    bool operator==(StringView rhs) const;

    StringView operator[](Range range) const;
    StringView operator[](RangeFrom range) const;
    StringView operator[](RangeTo range) const;

    char *begin();
    char *end();
    const char *begin() const;
    const char *end() const;
    usize len() const;

    template <typename S>
    friend S &operator<<(S &stream, const String &str);

private:
    Vector<char> mData;
};

struct StringOutputIterator
    : public std::iterator<std::output_iterator_tag, void, void, void, void> {
    explicit StringOutputIterator(String *output) : output(output) {}

    StringOutputIterator &operator=(char value) {
        output->push(value);
        return *this;
    }

    // clang-format off
    StringOutputIterator &operator*() { return *this; }
    StringOutputIterator &operator++() { return *this; }
    StringOutputIterator &operator++(int) { return *this; }
    // clang-format on

private:
    String *output;
};

StringOutputIterator String::insertLastIterator() {
    return StringOutputIterator{this};
}

struct StringView {
    View<const char> raw;

    StringView();
    StringView(const char *cstr);
    StringView(const char *str, usize len);
    StringView(View<const char> slice);
    StringView(String const &str);

    Option<usize> indexOf(StringView needle) const;
    Option<usize> indexOf(u32 needle) const;
    Option<usize> lastIndexOf(StringView needle) const;
    Option<usize> lastIndexOf(u32 needle) const;

    bool startsWith(StringView slice) const;
    bool startsWith(u32 codepoint) const;

    void split_to(Allocator &alloc, StringView token, Vector<String> &out) const;

    /// Note that both the vector and the strings within the vector are allocated using `alloc`.
    Vector<String> split(Allocator &alloc, StringView token) const;

    String to_string(Allocator &alloc) const;

    operator View<const char>() const { return raw; }

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
inline Vector<char> to_null_terminated(Allocator &alloc, StringView str);

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
Option<u32> decode(View<const char> buf);

bool is_valid(View<const char> buf);

} // namespace utf8

} // namespace vixen

inline ::vixen::StringView operator"" _s(const char *cstr, usize len) {
    return {{cstr, len}};
}

#include "string.inl"

// String foo = "1234";
// alloc(foo, myvar) = "1234";