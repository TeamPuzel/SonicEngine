// Created by Lua (TeamPuzel) on May 28th 2025.
// Copyright (c) 2025 All rights reserved.
//
// This header defines fixed point arithmetic used by the game.
//
// TODO(?): It would be nice to implement std::format for this type.
//          Not sure if that's worth my time since I have to port this game back to C++17 anyway.
#pragma once
#include "primitive.hpp"
#include <iostream>
#include <iomanip>

// Workarounds for the awful version of C++ I have to use, C++17.
namespace trash {
    #if defined(_MSC_VER) && !CROSS_COMPILING // We do not actually want to use this branch when using Clang after all.
    /// MSVC does not have a convenient builtin but it has botched semantics.
    ///
    /// From stack overflow:
    /// The member pointer equality operator works differently on compile time and runtime in visual studio.
    /// Compile time the equality checks the exact match (&N::a and &M::a are different, because of a different Base),
    /// but runtime it only checks the offset of the class (&N::a and &M::a are at the same offset: 0).
    /// int M::* and int N::* pointers are not comparable default, but if we create a common base class
    /// with the same offset, and use member casts (C++20 ISO: § 7.3.12, 2 and § 7.6.1.7, 12),
    /// they will comparable at the common C base class.
    ///
    /// This is completely stupid but does let us know.
    constexpr auto is_constant_evaluated() noexcept -> bool {
        struct C {};
        struct M : C { int a; };
        struct N : C { int a; };

        return &M::a != static_cast<int C::*>(&N::a);
    }
    #else
    /// With clang we can just use the builtin despite being in a prior C++ version than this is
    /// exposed. Technically it means this would not compile with old C++17 clang but who cares.
    /// I do not, they are long unsupported. Using C++17 in any new code is genuinely stupid.
    /// These "versions" are just toggling some features, one can compile code for a project
    /// combining language modes, it's the same compiler ffs. What a waste of my time.
    ///
    /// I suppose GCC also exists but I don't like GCC. I dislike MSVC even more but I have to support it sadly.
    constexpr auto is_constant_evaluated() noexcept -> bool {
        return __builtin_is_constant_evaluated();
    }
    #endif
}

/// A fixed point numeric type used by the game to more accurately recreate the original physics.
/// It is a 24 bit signed integer used together with an 8 bit fractional part.
/// The sign is expressed as two's complement.
/// It implements the usual implicit conversions to behave much like existing primitives.
///
/// That's because the original SEGA hardware did not have floats and had to use subpixels, a decimal
/// type with 8 bit decimal precision. This type should result in math very accurate to the original
/// and maintain an unchanging level of precision as the stage goes on.
///
/// Much like signed primitives of C++ its overflow is undefined.
///
/// The type relies on the static assertion of the core header, that signed primitives are two's complement
/// and the compiler performs sign extension when lowering an arithmetic shift.
///
/// Because C++17 is an archaic decade old version and can't do signed shifts in constexpr I use equally awful compiler
/// abuse to get around it. This language before C++20/23/26 is genuinely sad.
class [[clang::trivial_abi]] fixed final { // NOLINT(readability-identifier-naming)
    u32 raw;

    constexpr explicit fixed(u32 raw) noexcept : raw(raw) {}

  public:
    [[clang::always_inline]]
    constexpr fixed() : raw(0) {}

    /// Constructs a fixed type from an integer and fraction part.
    ///
    /// The i32 type is the best fit to represent the 24 bit number and the sign is taken into account.
    ///
    /// It is intentionally implicit, using an integer or an integer literal is sound here.
    [[clang::always_inline]]
    constexpr fixed(i32 whole, u8 fraction = 0) noexcept : raw(0) {
        // This garbage code means we get constexpr in C++17 while having a more sensible runtime implementation.
        if constexpr (trash::is_constant_evaluated()) {
            const bool negative = whole < 0;
            const u32 offset = (i32(fraction) ^ -i32(negative)) + negative;
            this->raw = u32(whole * 256 + offset);
        } else {
            this->raw = u32((whole << 8) + ((i32(fraction) ^ (whole >> 31)) - (whole >> 31)));
        }
    }

    [[clang::always_inline]]
    constexpr explicit operator i32() const noexcept {
        // Casting to a signed type will perform an arithmetic shift instead and preserve the sign
        return i32(raw) >> 8;
    }

    [[clang::always_inline]]
    static constexpr fixed from_raw(i32 value) noexcept {
        return fixed((u32) value);
    }

    [[clang::always_inline]]
    static constexpr auto into_raw(fixed value) noexcept -> i32 {
        return i32(value.raw);
    }

    friend std::ostream& operator<<(std::ostream& os, const fixed& value) {
        i32 whole = i32(value);
        u8 fraction = whole > 0 ? u8(value.raw & 0xFF) : 256 - u8(value.raw & 0xFF);

        os << whole << '.'
           << std::setfill('0') << std::setw(3) << i32(fraction);

        return os;
    }
};

static_assert(sizeof(fixed) == 4);
static_assert(alignof(fixed) == 4);

/// A more explicitly named alias of the fixed type.
using i24d8 = fixed;

[[clang::always_inline]]
constexpr auto operator==(fixed lhs, fixed rhs) noexcept -> bool {
    return fixed::into_raw(lhs) == fixed::into_raw(rhs);
}

[[clang::always_inline]]
constexpr auto operator!=(fixed lhs, fixed rhs) noexcept -> bool {
    return fixed::into_raw(lhs) != fixed::into_raw(rhs);
}

[[clang::always_inline]]
constexpr auto operator<(fixed lhs, fixed rhs) noexcept -> bool {
    return fixed::into_raw(lhs) < fixed::into_raw(rhs);
}

[[clang::always_inline]]
constexpr auto operator<=(fixed lhs, fixed rhs) noexcept -> bool {
    return fixed::into_raw(lhs) <= fixed::into_raw(rhs);
}

[[clang::always_inline]]
constexpr auto operator>(fixed lhs, fixed rhs) noexcept -> bool {
    return fixed::into_raw(lhs) > fixed::into_raw(rhs);
}

[[clang::always_inline]]
constexpr auto operator>=(fixed lhs, fixed rhs) noexcept -> bool {
    return fixed::into_raw(lhs) >= fixed::into_raw(rhs);
}

[[clang::always_inline]]
constexpr auto operator+(fixed lhs, fixed rhs) noexcept -> fixed {
    return fixed::from_raw(fixed::into_raw(lhs) + fixed::into_raw(rhs));
}

[[clang::always_inline]]
constexpr auto operator-(fixed lhs, fixed rhs) noexcept -> fixed {
    return fixed::from_raw(fixed::into_raw(lhs) - fixed::into_raw(rhs));
}

[[clang::always_inline]]
constexpr auto operator-(fixed self) noexcept -> fixed {
    // Note that negating an unsigned integer is actually defined in this language.
    // It's -n == 0 - n
    // This behavior matches the two's complement representation we want.
    return fixed::from_raw(-fixed::into_raw(self));
}

[[clang::always_inline]]
constexpr auto operator+(fixed self) noexcept -> fixed {
    return self;
}

[[clang::always_inline]]
constexpr auto operator*(fixed lhs, fixed rhs) noexcept -> fixed {
    i64 result = i64(fixed::into_raw(lhs)) * fixed::into_raw(rhs);
    return fixed::from_raw(i32(result >> 8));
}

[[clang::always_inline]]
constexpr auto operator/(fixed lhs, fixed rhs) noexcept -> fixed {
    i64 numerator = i64(fixed::into_raw(lhs)) << 8;
    i32 denominator = fixed::into_raw(rhs);
    return fixed::from_raw(i32(numerator / denominator));
}

[[clang::always_inline]]
constexpr void operator+=(fixed& lhs, fixed rhs) noexcept {
    lhs = lhs + rhs;
}

[[clang::always_inline]]
constexpr void operator-=(fixed& lhs, fixed rhs) noexcept {
    lhs = lhs - rhs;
}

[[clang::always_inline]]
constexpr void operator*=(fixed& lhs, fixed rhs) noexcept {
    lhs = lhs * rhs;
}

[[clang::always_inline]]
constexpr void operator/=(fixed& lhs, fixed rhs) noexcept {
    lhs = lhs / rhs;
}

namespace math {
    [[clang::always_inline]]
    constexpr auto trunc(fixed value) noexcept -> fixed {
        return fixed((i32) value);
    }

    [[clang::always_inline]]
    constexpr auto abs(fixed value) noexcept -> fixed {
        i32 signed_raw = fixed::into_raw(value);
        i32 abs_raw = signed_raw < 0 ? -signed_raw : signed_raw;
        return fixed::from_raw(abs_raw);
    }

    [[clang::always_inline]]
    constexpr auto sign(fixed value) noexcept -> fixed {
        if (value == 0) {
            return 0;
        } else if (value > 0) {
            return 1;
        } else {
            return -1;
        }
    }

    [[clang::always_inline]]
    constexpr auto floor(fixed value) noexcept -> fixed {
        const i32 raw = fixed::into_raw(value);
        const bool is_negative = raw < 0;
        const bool has_fraction = (raw & 0xFF) != 0;

        // If negative and has a fraction, subtract 1 from the integer part
        const i32 floored = (raw >> 8) - (is_negative && has_fraction ? 1 : 0);
        return floored;
    }
}
