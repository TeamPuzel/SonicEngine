// Created by Lua (TeamPuzel) on May 29th 2025.
// Copyright (c) 2025 All rights reserved.
//
// This header defines color functionality such as blending.
// The sonic game does not need blending so only the binary blending mode is implemented.
#pragma once
#include <primitive>

namespace draw {
    struct Color final {
        u8 r, g, b, a;

        static constexpr Color rgba(u8 r, u8 g, u8 b, u8 a = 255) noexcept {
            return Color { r, g, b, a };
        }

        template <typename Blend> constexpr auto blend_over(Color other, Blend const& blend) const noexcept -> Color {
            return blend(*this, other);
        }

        template <typename Blend> constexpr auto blend_under(Color other, Blend const& blend) const noexcept -> Color {
            return blend(other, *this);
        }

        constexpr auto operator==(Color other) const noexcept -> bool {
            return r == other.r and g == other.g and b == other.b and a == other.a;
        }

        constexpr auto operator!=(Color other) const noexcept -> bool {
            return !(*this == other);
        }

        constexpr auto with_r(u8 r) const noexcept -> Color {
            return { r, g, b, a };
        }

        constexpr auto with_g(u8 g) const noexcept -> Color {
            return { r, g, b, a };
        }

        constexpr auto with_b(u8 b) const noexcept -> Color {
            return { r, g, b, a };
        }

        constexpr auto with_a(u8 a) const noexcept -> Color {
            return { r, g, b, a };
        }
    };

    namespace blend {
        constexpr auto overwrite(Color top, Color bottom) noexcept -> Color {
            return top;
        }

        /// The default style of blending, if any transparency is present the color is discarded
        /// completely and no blending is actually performed.
        ///
        /// This is a great default because it remains associative unlike more advanced alpha blending.
        constexpr auto binary(Color top, Color bottom) noexcept -> Color {
            return top.a == 255 ? top : bottom;
        }

        /// Alpha blending, not intended for use by the game since the original hardware didn't support
        /// transparency, however it could be useful for transparent debug overlays.
        constexpr auto alpha(Color top, Color bottom) noexcept -> Color {
            const i32 tr = top.r, tg = top.g, tb = top.b, ta = top.a;
            const i32 br = bottom.r, bg = bottom.g, bb = bottom.b, ba = bottom.a;
            const i32 inv_a = 255 - ta;

            return Color::rgba(
                (tr * ta + br * inv_a) / 255,
                (tg * ta + bg * inv_a) / 255,
                (tb * ta + bb * inv_a) / 255,
                (ta + (ba * inv_a) / 255)
            );
        }
    }

    namespace color {
        constexpr Color CLEAR = Color::rgba(0, 0, 0, 0);
        constexpr Color WHITE = Color::rgba(255, 255, 255);
        constexpr Color BLACK = Color::rgba(0, 0, 0);
    }

    namespace color::pico {
        constexpr Color BLACK       = Color::rgba(0,   0,   0  );
        constexpr Color DARK_BLUE   = Color::rgba(29,  43,  83 );
        constexpr Color DARK_PURPLE = Color::rgba(126, 37,  83 );
        constexpr Color DARK_GREEN  = Color::rgba(0,   135, 81 );
        constexpr Color BROWN       = Color::rgba(171, 82,  53 );
        constexpr Color DARK_GRAY   = Color::rgba(95,  87,  79 );
        constexpr Color LIGHT_GRAY  = Color::rgba(194, 195, 199);
        constexpr Color WHITE       = Color::rgba(255, 241, 232);
        constexpr Color RED         = Color::rgba(255, 0,   77 );
        constexpr Color ORANGE      = Color::rgba(255, 163, 0  );
        constexpr Color YELLOW      = Color::rgba(255, 236, 39 );
        constexpr Color GREEN       = Color::rgba(0,   228, 54 );
        constexpr Color BLUE        = Color::rgba(41,  173, 255);
        constexpr Color LAVENDER    = Color::rgba(131, 118, 156);
        constexpr Color PINK        = Color::rgba(255, 119, 168);
        constexpr Color PEACH       = Color::rgba(255, 204, 170);
    }
}
