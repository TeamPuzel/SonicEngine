// Created by Lua (TeamPuzel) on May 29th 2025.
// Copyright (c) 2025 All rights reserved.
//
// This defines the simplest possible primitive.
// It's not the most primitive however, that title belongs to the InfiniteImage.
//
// TODO:
// - Replace std::vector with core::Array to guarantee valid zero initialization.
#pragma once
#include <primitive>
#include <vector>
#include "Drawable.hpp"

namespace draw {
    /// The simplest sized primitive, it makes for a general purpose read/write drawable.
    ///
    /// The state surrounding the described sized area is always clear.
    ///
    /// The empty state of an image is equivalent to zero initialization.
    class Image final {
        std::vector<Color> data;
        i32 w, h;

      public:
        /// The default, empty image of nil proportions.
        Image() : w(0), h(0) {}

        Image(i32 width, i32 height, Color default_color = color::CLEAR) : w(width), h(height) {
            data.reserve(width * height);
            for (i32 x = 0; x < width; x += 1) {
                for (i32 y = 0; y < height; y += 1) {
                    data.push_back(default_color);
                }
            }
        }

        /// Initializes the image with the provided function of signature:
        /// (x: i32, y: i32) -> Color
        Image(i32 width, i32 height, auto init) : w(width), h(height) {
            data.reserve(width * height);
            for (i32 x = 0; x < width; x += 1) {
                for (i32 y = 0; y < height; y += 1) {
                    data.push_back(init(x, y));
                }
            }
        }

        void resize(i32 width, i32 height) {
            *this = Image(width, height, [this] (i32 x, i32 y) -> Color {
                return this->get(x, y);
            });
        }

        auto width() const -> i32 {
            return w;
        }

        auto height() const -> i32 {
            return h;
        }

        auto get(i32 x, i32 y) const -> Color {
            if (x >= 0 and x < w and y >= 0 and y < h) {
                return data[x + y * w];
            } else {
                return color::CLEAR;
            }
        }

        void set(i32 x, i32 y, Color color) {
            if (x >= 0 and x < w and y >= 0 and y < h) {
                data[x + y * w] = color;
            }
        }

        auto raw() const -> Color const* {
            return data.data();
        }

        auto raw() -> Color* {
            return data.data();
        }
    };

    static_assert(SizedMutableDrawable<Image>);
}
