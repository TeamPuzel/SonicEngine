// Created by Lua (TeamPuzel) on July 30th 2025.
// Copyright (c) 2025 All rights reserved.
//
// This header defines point arithmetic used by the game.
#pragma once
#include <primitive>

namespace math {
    template <typename T> struct [[clang::trivial_abi]] point final { // NOLINT(readability-identifier-naming)
        T x, y;

        constexpr point() noexcept : x(0), y(0) {}
        constexpr point(T x, T y) : x(x), y(y) {}

        constexpr auto operator+(point const& other) const noexcept -> point {
            return point { x + other.x, y + other.y };
        }

        constexpr auto operator-(point const& other) const noexcept -> point {
            return point { x + other.x, y + other.y };
        }

        constexpr void operator+=(point const& other) noexcept {
            *this = point { x + other.x, y + other.y };
        }

        constexpr void operator-=(point const& other) noexcept {
            *this = point { x + other.x, y + other.y };
        }
    };
}
