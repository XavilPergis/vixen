#include "vixen/string.hpp"

#include "vixen/util.hpp"

namespace vixen {
namespace utf8 {
void encode(u32 codepoint, char *buf) {
    // NOTE: This function ensures codepoint is valid.
    usize len = encoded_len(codepoint);
    switch (len) {
    // U+0000 though U+007F are encoded as single octets with the same value as the codepoint.
    case 1: buf[0] = codepoint; break;
    case 2:
        buf[0] = 0xc0 | (codepoint & 0x1f << 6) >> 6; // bits 7 through 11
        buf[1] = 0x80 | (codepoint & 0x3f); // bits 0 through 6
        break;
    case 3:
        buf[0] = 0xe0 | (codepoint & 0x0f << 12) >> 12; // bits 13 through 16
        buf[1] = 0x80 | (codepoint & 0x3f << 6) >> 6; // bits 7 through 12
        buf[2] = 0x80 | (codepoint & 0x3f); // bits 0 through 6
        break;
    case 4:
        buf[0] = 0xf0 | (codepoint & 0x05 << 18) >> 18; // bits 19 through 21
        buf[1] = 0x80 | (codepoint & 0x3f << 12) >> 12; // bits 13 through 18
        buf[2] = 0x80 | (codepoint & 0x3f << 6) >> 6; // bits 7 through 12
        buf[3] = 0x80 | (codepoint & 0x3f); // bits 0 through 6
        break;
    default:
        VIXEN_UNREACHABLE("UTF-8 encoded length of {} for codepoint U+{:X} is wrong.",
            len,
            codepoint);
    }
}

static Option<usize> get_seq_length(char octet) {
    // clang-format off
    if      ((octet & 0x80) == 0)    { return usize(1);  }
    else if ((octet & 0xe0) == 0xc0) { return usize(2);  }
    else if ((octet & 0xf0) == 0xe0) { return usize(3);  }
    else if ((octet & 0xf8) == 0xf0) { return usize(4);  }
    else                             { return {}; }
    // clang-format on
}

Option<u32> decode(View<const char> buf) {
    if (buf.len() == 0) {
        return {};
    }

    if (auto len = get_seq_length(buf[0])) {
        if (*len > buf.len()) {
            return {};
        }

        u32 codepoint = 0;

        switch (*len) {
        case 1: codepoint = buf[0]; break;
        case 2:
            codepoint |= (buf[0] & 0x1f) << 6;
            codepoint |= (buf[1] & 0x3f);
            break;
        case 3:
            codepoint |= (buf[0] & 0x0f) << 12;
            codepoint |= (buf[1] & 0x3f) << 6;
            codepoint |= (buf[2] & 0x3f);
            break;
        case 4:
            codepoint |= (buf[0] & 0x05) << 18;
            codepoint |= (buf[1] & 0x3f) << 12;
            codepoint |= (buf[2] & 0x3f) << 6;
            codepoint |= (buf[3] & 0x3f);
            break;
        }
        return codepoint;
    }

    return {};
}

#define MATCH_RANGE(o, s, e)                                                                       \
    valid                                                                                          \
        &= static_cast<u8>(data[current + o]) >= (s) && static_cast<u8>(data[current + o]) <= (e); \
    ++current;
#define IN_RANGE(s, e) \
    (static_cast<u8>(data[current]) >= (s) && static_cast<u8>(data[current]) <= (e))
#define IN_BOUNDS(l) ((current + (l)-1) < data.len())

bool is_valid(View<const char> data) {
    usize current = 0;

    while (current < data.len()) {
        bool valid = true;

        // The valid range for a sinle byte encoding is 0 through 0x7f inclusive, which exactly
        // matches the range of values we get after masking off the top bit, so anything starting
        // with a zero will always be valid.
        if ((data[current] & 0x80) == 0) {
            ++current;
            continue;
        }

        // ===== DOUBLE WIDE =====
        // C2..DF / 80..BF
        else if (IN_BOUNDS(2) && IN_RANGE(0xc2, 0xdf))
        {
            MATCH_RANGE(1, 0x80, 0xbf)
        }

        // ===== TRIPLE WIDE =====
        // E0 / A0..BF / 80..BF
        else if (IN_BOUNDS(3) && static_cast<u8>(data[current]) == 0xe0)
        {
            MATCH_RANGE(1, 0xc2, 0xbf)
            MATCH_RANGE(2, 0x80, 0xbf)
        }

        // E1..EC / 80..BF / 80..BF
        else if (IN_BOUNDS(3) && IN_RANGE(0xe1, 0xec))
        {
            MATCH_RANGE(1, 0x80, 0xbf)
            MATCH_RANGE(2, 0x80, 0xbf)
        }

        // ED / 80..9F / 80..BF
        else if (IN_BOUNDS(3) && static_cast<u8>(data[current]) == 0xed)
        {
            MATCH_RANGE(1, 0x80, 0x9f)
            MATCH_RANGE(2, 0x80, 0xbf)
        }

        // EE..EF / 80..BF / 80..BF
        else if (IN_BOUNDS(3) && IN_RANGE(0xee, 0xef))
        {
            MATCH_RANGE(1, 0x80, 0xbf)
            MATCH_RANGE(2, 0x80, 0xbf)
        }

        // ===== QUADRUPLE WIDE =====
        // F0 / 90..BF / 80..BF / 80..BF
        else if (IN_BOUNDS(4) && static_cast<u8>(data[current]) == 0xf0)
        {
            MATCH_RANGE(1, 0x90, 0xbf)
            MATCH_RANGE(2, 0x80, 0xbf)
            MATCH_RANGE(3, 0x80, 0xbf)
        }

        // F1..F3 / 80..BF / 80..BF / 80..BF
        else if (IN_BOUNDS(4) && IN_RANGE(0xf1, 0xf3))
        {
            MATCH_RANGE(1, 0x80, 0xbf)
            MATCH_RANGE(2, 0x80, 0xbf)
            MATCH_RANGE(3, 0x80, 0xbf)
        }

        // F0 / 90..BF / 80..BF / 80..BF
        else if (IN_BOUNDS(4) && static_cast<u8>(data[current]) == 0xf0)
        {
            MATCH_RANGE(1, 0x90, 0xbf)
            MATCH_RANGE(2, 0x80, 0xbf)
            MATCH_RANGE(3, 0x80, 0xbf)
        }

        // Anything else, including C0 and C1
        else
        {
            valid = false;
        }

        if (!valid) {
            return false;
        }
    }

    return true;
}

} // namespace utf8

} // namespace vixen
