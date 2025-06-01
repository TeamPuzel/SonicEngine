// Created by Lua (TeamPuzel) on May 25th 2025.
// Copyright (c) 2025 All rights reserved.
//
// Primitive aliases asserted to a specific size representation.
// C++ is still defined loosely enough that on exotic platforms these would be invalid,
// but for all modern platforms I care about this should be  enough to guarantee consistency.
//
// This avoids common bugs like assuming signedness or size of primitives, which does differ between platforms.
#pragma once

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
