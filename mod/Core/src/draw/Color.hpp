// Created by Lua (TeamPuzel) on May 29th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <core>

namespace core::draw {
    struct Color final {
        u8 r, g, b, a;
    };
    
    namespace pal {
        constexpr Color CLEAR = { 0, 0, 0, 0 };
        constexpr Color WHITE = { 255, 255, 255, 255 };
        constexpr Color BLACK = { 0, 0, 0, 255 };
    }
}
