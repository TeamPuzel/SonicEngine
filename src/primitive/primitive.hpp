// Created by Lua (TeamPuzel) on May 25th 2025.
// Copyright (c) 2025 All rights reserved.
//
// Primitive aliases asserted to a specific memory representation.
// C++ is still defined loosely enough that on exotic platforms these would be invalid,
// but for all modern platforms I care about this should be enough to guarantee consistency.
//
// This avoids common bugs like assuming signedness or size of primitives, which does differ between platforms.
// The header also asserts that certain poorly defined operations evaluate as expected.
#pragma once
#include <type_traits>

using f32 = float;
using f64 = double;

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;

using i8 = signed char;
using i16 = signed short;
using i32 = signed int;

#ifdef OS_WINDOWS
using u64 = unsigned long long;
using i64 = signed long long;
#else
using u64 = unsigned long;
using i64 = signed long;
#endif

#if POINTER_BIT_WIDTH == 32
using usize = u32;
using isize = i32;
#else
using usize = u64;
using isize = i64;
#endif

static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);

static_assert(sizeof(u8) == 1);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(u64) == 8);

static_assert(sizeof(i8) == 1);
static_assert(sizeof(i16) == 2);
static_assert(sizeof(i32) == 4);
static_assert(sizeof(i64) == 8);

static_assert(sizeof(usize) == sizeof(void*));
static_assert(sizeof(isize) == sizeof(void*));

// This is required by C++20, however it technically isn't in prior versions.
// Since this code has to be ported back to C++17 assert that we're not using a weird compiler.
static_assert(-4 >> 1 == -2, ">> doesn't do sign extension");

// C++ does not define the representation of signed primitives either.
// Assert that this is a modern and sensible compiler/architecture.
// This is a requirement. Code can rely on this representation from this point on.
static_assert(static_cast<i8>(-1) == ~i8(0), "Compiler is not using two's complement");
static_assert(static_cast<i32>(-1) == ~i32(0), "Compiler is not using two's complement");
static_assert(static_cast<i64>(-1) == ~i64(0), "Compiler is not using two's complement");

namespace endian {
    template <typename T, typename... Bytes> constexpr auto from_le_bytes(Bytes... bytes) noexcept -> T {
        static_assert(sizeof...(Bytes) == sizeof(T), "Incorrect number of bytes");
        static_assert(std::is_integral<T>::value, "T must be an integral type");

        T value = 0;
        u8 data[] = { static_cast<u8>(bytes)... };
        for (usize i = 0; i < sizeof(T); i += 1) value |= data[i] << (i * 8);
        return value;
    }

    template <typename T, typename... Bytes> constexpr auto from_be_bytes(Bytes... bytes) noexcept -> T {
        static_assert(sizeof...(Bytes) == sizeof(T), "Incorrect number of bytes");
        static_assert(std::is_integral<T>::value, "T must be an integral type");

        T value = 0;
        u8 data[] = { static_cast<u8>(bytes)... };
        for (usize i = 0; i < sizeof(T); i += 1) value |= data[i] << ((sizeof(T) - 1 - i) * 8);
        return value;
    }
}
