// Created by Lua (TeamPuzel) on May 29th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <primitive>

namespace draw {
    struct Color final {
        u8 r, g, b, a;

        static constexpr Color rgba(u8 r, u8 g, u8 b, u8 a = 255) {
            return Color { r, g, b, a };
        }
    };

    namespace color {
        constexpr Color CLEAR = Color::rgba(0, 0, 0, 0);
        constexpr Color WHITE = Color::rgba(255, 255, 255);
        constexpr Color BLACK = Color::rgba(0, 0, 0, 255);
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
