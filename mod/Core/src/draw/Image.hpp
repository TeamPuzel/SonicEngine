// Created by Lua (TeamPuzel) on May 29th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <core>
#include "Drawable.hpp"

namespace core::draw {
    class Image final {
        core::Array<Color> data;
        i32 w, h;

      public:
        auto width() const -> i32 {
            return w;
        }

        auto height() const -> i32 {
            return h;
        }

        auto get(i32 x, i32 y) const -> Color {
            core::todo();
        }

        void set(i32 x, i32 y, Color color) {
            core::todo();
        }
    };

    static_assert(SizedMutableDrawable<Image>);
}
