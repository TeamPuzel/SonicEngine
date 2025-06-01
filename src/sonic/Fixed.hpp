// Created by Lua (TeamPuzel) on May 28th 2025.
// Copyright (c) 2025 All rights reserved.
//
// This header defines fixed point arithmetic used by the game.
//
// TODO(?): It would be nice to implement std::format for this type.
//          Not sure if that's worth my time since I have to port this game back to C++17 anyway.
// TODO(?): This type could be signed.
// TODO(!): Operator overloads for mixing fixed types with integers.
#pragma once
#include <core>

namespace sonic {
    /// A fixed point number type used by the game to more accurately recreate the original physics.
    /// It is a 24 bit unsigned integer used together with an 8 bit fractional part.
    /// It implements the usual implicit conversions to behave much like existing primitives.
    ///
    /// That's because the original hardware did not have floats and had to use subpixels, a decimal
    /// type with 8 bit decimal precision. This should result in math very accurate to the original
    /// and preserve the same level of precision as the stage goes on, where floats would slowly lose precision.
    ///
    /// Much like signed primitives of C++ its overflow is undefined.
    class [[clang::trivial_abi]] fixed final { // NOLINT(readability-identifier-naming)
        u32 raw;

      public:
        [[clang::always_inline]]
        constexpr explicit(false) fixed(u32 whole, u8 fraction = 0)
            : raw((whole << 8) | fraction) {}

        [[clang::always_inline]] constexpr explicit(false) operator u32() const {
            return raw >> 8;
        }

        [[clang::always_inline]]
        static constexpr fixed from_raw(u32 value) {
            return fixed(value >> 8, value & 0xFF);
        }

        [[clang::always_inline]]
        static constexpr auto into_raw(fixed value) -> u32 {
            return value.raw;
        }
    };

    static_assert(sizeof(fixed) == 4);
    static_assert(alignof(fixed) == 4);

    [[clang::always_inline]]
    constexpr auto operator==(fixed lhs, fixed rhs) -> bool {
        return fixed::into_raw(lhs) == fixed::into_raw(rhs);
    }

    [[clang::always_inline]]
    constexpr auto operator!=(fixed lhs, fixed rhs) -> bool {
        return fixed::into_raw(lhs) == fixed::into_raw(rhs);
    }

    [[clang::always_inline]]
    constexpr auto operator<(fixed lhs, fixed rhs) -> bool {
        return fixed::into_raw(lhs) < fixed::into_raw(rhs);
    }

    [[clang::always_inline]]
    constexpr auto operator<=(fixed lhs, fixed rhs) -> bool {
        return fixed::into_raw(lhs) <= fixed::into_raw(rhs);
    }

    [[clang::always_inline]]
    constexpr auto operator>(fixed lhs, fixed rhs) -> bool {
        return fixed::into_raw(lhs) > fixed::into_raw(rhs);
    }

    [[clang::always_inline]]
    constexpr auto operator>=(fixed lhs, fixed rhs) -> bool {
        return fixed::into_raw(lhs) >= fixed::into_raw(rhs);
    }

    [[clang::always_inline]]
    constexpr auto operator+(fixed lhs, fixed rhs) -> fixed {
        return fixed::from_raw(fixed::into_raw(lhs) + fixed::into_raw(rhs));
    }

    [[clang::always_inline]]
    constexpr auto operator-(fixed lhs, fixed rhs) -> fixed {
        return fixed::from_raw(fixed::into_raw(lhs) - fixed::into_raw(rhs));
    }

    [[clang::always_inline]]
    constexpr auto operator*(fixed lhs, fixed rhs) -> fixed {
        u32 a = fixed::into_raw(lhs);
        u32 b = fixed::into_raw(rhs);
        u64 result = u64(a) * b; // 64-bit to avoid overflow
        return fixed::from_raw(u32(result >> 8));
    }

    [[clang::always_inline]]
    constexpr auto operator/(fixed lhs, fixed rhs) -> fixed {
        u32 a = fixed::into_raw(lhs);
        u32 b = fixed::into_raw(rhs);
        u64 numerator = u64(a) << 8;
        return fixed::from_raw(u32(numerator / b));
    }

    [[clang::always_inline]]
    constexpr void operator+=(fixed& lhs, fixed rhs) {
        lhs = lhs + rhs;
    }

    [[clang::always_inline]]
    constexpr void operator-=(fixed& lhs, fixed rhs) {
        lhs = lhs - rhs;
    }

    [[clang::always_inline]]
    constexpr void operator*=(fixed& lhs, fixed rhs) {
        lhs = lhs * rhs;
    }

    [[clang::always_inline]]
    constexpr void operator/=(fixed& lhs, fixed rhs) {
        lhs = lhs / rhs;
    }
}
