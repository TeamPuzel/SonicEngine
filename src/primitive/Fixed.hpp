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
#include "Primitive.hpp"

// Workarounds for the garbage version of C++ I have to use, C++17.
namespace trash {
    #ifdef _MSC_VER
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
/// Because C++17 is an archaic decade old version and can't do signed shifts in constexpr I use equally garbage compiler
/// abuse to get around it. This language before C++20/23 is genuinely just pathetic garbage.
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
    constexpr operator i32() const noexcept {
        // Casting to a signed type will perform an arithmetic shift instead and preserve the sign
        return i32(raw) >> 8;
    }

    [[clang::always_inline]]
    static constexpr fixed from_raw(u32 value) noexcept {
        return fixed(value);
    }

    [[clang::always_inline]]
    static constexpr auto into_raw(fixed value) noexcept -> u32 {
        return value.raw;
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
    return fixed::into_raw(lhs) == fixed::into_raw(rhs);
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
constexpr auto operator*(fixed lhs, fixed rhs) noexcept -> fixed {
    u32 a = fixed::into_raw(lhs);
    u32 b = fixed::into_raw(rhs);
    u64 result = u64(a) * b; // 64-bit to avoid overflow
    return fixed::from_raw(u32(result >> 8));
}

[[clang::always_inline]]
constexpr auto operator/(fixed lhs, fixed rhs) noexcept -> fixed {
    u32 a = fixed::into_raw(lhs);
    u32 b = fixed::into_raw(rhs);
    u64 numerator = u64(a) << 8;
    return fixed::from_raw(u32(numerator / b));
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
