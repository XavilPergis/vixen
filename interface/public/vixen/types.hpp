#pragma once

#include <cstddef>
#include <cstdint>

// pointer-sized numeric types. *intptr types can hold pointers, and *size can hold pointer offsets.
using uintptr = std::uintptr_t;
using intptr = std::intptr_t;
using usize = std::size_t;
using isize = std::ptrdiff_t;

// specific-width numeric types
using u64 = std::uint64_t;
using i64 = std::int64_t;
using u32 = std::uint32_t;
using i32 = std::int32_t;
using u16 = std::uint16_t;
using i16 = std::int16_t;
using u8 = std::uint8_t;
using i8 = std::int8_t;

using opaque = std::byte;

using f32 = float;
using f64 = double;
