// Created by Lua (TeamPuzel) on May 29th 2025.
// Copyright (c) 2025 All rights reserved.
//
// This defines the simplest possible primitive.
// It's not the most primitive however, that title belongs to the InfiniteImage.
#pragma once
#include <primitive>
#include <string_view>
#include "color.hpp"
#include "plane.hpp"
#include "image.hpp"

namespace draw {
    template <typename T> struct Symbol final {
        static_assert(Plane<T>::value);

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

        Symbol(Glyph glyph) : type(Type::Glyph), glyph(glyph) {}
        Symbol(Space space) : type(Type::Space), space(space) {}

        auto width() const -> i32 {
            switch (type) {
                case Type::Glyph: return glyph.width();
                case Type::Space: return space.width;
            }
        }
    };

    template <typename T, typename Chr> struct Font final {
        static_assert(Plane<T>::value);

        T source;
        i32 height;
        i32 baseline;
        i32 spacing;
        i32 leading;
        auto (*map) (T const&, Chr) -> Symbol<T>;

        auto symbol(Chr c) const -> Symbol<T> {
            return this->map(source, c);
        }
    };

    /// A drawable representing text.
    ///
    /// Notably this type performs caching to be remotely efficient while maintaining the composable
    /// drawable interface. This type is not thread-safe and has to be guarded if shared in any way.
    ///
    /// The view type used is generic because C++17 is sad and as usual the version prior to anything nice.
    /// Not that it's much better now anyway.
    template <typename T, typename Str = std::string_view> struct Text final {
        using StringView = Str;
        using Char = typename Str::value_type;

        StringView content;
        Color color;
        Font<T, Char> font;

      private:
        i32 width_cache;
        mutable std::optional<Image> cache;

        auto redraw() const -> Image {
            using SymbolType = typename Symbol<T>::Type;

            auto ret = Image(width(), height());

            i32 cursor = 0;

            for (Char c : content) {
                auto sym = font.symbol(c);
                switch (sym.type) {
                    case SymbolType::Glyph:
                        ret | draw(
                            sym.glyph | draw::map([this] (Color c, i32 x, i32 y) -> Color {
                                return c == color::WHITE ? color : c;
                            }),
                            cursor, 0,
                            blend::overwrite
                        );

                        cursor += sym.width() + font.spacing;
                        break;
                    case SymbolType::Space:
                        cursor += sym.width();
                        break;
                }
            }

            return ret;
        }

      public:
        Text(StringView content, Font<T, Char> font, Color color = color::WHITE)
            : content(content), color(color), font(font)
        {
            if (content.empty()) {
                this->width_cache = 0;
            } else {
                i32 acc = -font.spacing;
                for (Char c : content) acc += font.symbol(c).width() + font.spacing;
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

    template <typename T> Text(char const*, Font<T, char>, Color = color::WHITE) -> Text<T, std::string_view>;

    static_assert(SizedPlane<Text<Image>>::value);
}
