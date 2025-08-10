// Created by Lua (TeamPuzel) on May 29th 2025.
// Copyright (c) 2025 All rights reserved.
//
// This defines the simplest possible primitive.
// It's not the most primitive however, that title belongs to the InfiniteImage.
#pragma once
#include <primitive>
#include <string_view>
#include "drawable.hpp"
#include "image.hpp"

namespace draw {
    template <typename T> struct Symbol final {
        static_assert(Drawable<T>::value);

        enum class Type {
            Glyph,
            Space,
        } type;

        using Glyph = Slice<T>;
        struct Space final { i32 width; };

        union {
            Glyph glyph;
            Space space;
        };

        auto width() const -> i32 {
            switch (type) {
                case Type::Glyph: return glyph.width();
                case Type::Space: return space.width;
            }
        }
    };

    template <typename T> struct Font final {
        static_assert(Drawable<T>::value);

        T source;
        i32 height;
        i32 baseline;
        i32 spacing;
        i32 leading;
        auto(map) (T const&, char) -> Symbol<T>;

        auto symbol(char c) const -> Symbol<T> {
            return this->map(source, c);
        }
    };

    /// A drawable representing text.
    ///
    /// Notably this type performs caching to be remotely efficient while maintaining the composable
    /// drawable interface. This type is not thread-safe and has to be guarded if shared in any way.
    template <typename T> struct Text final {
        std::string_view content;
        Color color;
        Font<T> font;

      private:
        i32 width_cache;
        mutable std::optional<Image> cache;

        auto redraw() const -> Image {
            using SymbolType = typename Symbol<T>::Type;

            auto ret = Image(width(), height());

            i32 cursor = 0;

            for (char c : content) {
                auto sym = font.symbol(c);
                switch (sym.type) {
                    case SymbolType::Glyph:
                        // TODO: Map white to the text color.
                        ret | draw(sym.glyph, cursor, 0, blend::overwrite);

                        cursor += sym.glyph.width + font.spacing;
                        break;
                    case SymbolType::Space:
                        cursor += sym.space.width;
                        break;
                }
            }

            return ret;
        }

      public:
        Text(std::string_view content, Font<T> font, Color color = color::WHITE)
            : content(content), color(color), font(font)
        {
            if (content.empty()) {
                this->width_cache = 0;
            } else {
                i32 acc = -font.spacing;
                for (char c : content) acc += font.symbol(c).width() + font.spacing;
                this->width_cache = acc;
            }
        }

        auto width() const -> i32 {
            return width_cache;
        }

        auto height() const -> i32 {
            return font.height;
        }

        auto get(i32 x, i32 y) const -> Color {
            if (not cache) cache = redraw();
            return cache->get(x, y);
        }
    };

    static_assert(SizedDrawable<Text<Image>>::value);
}
