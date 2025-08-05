// Created by Lua (TeamPuzel) on May 29th 2025.
// Copyright (c) 2025 All rights reserved.
//
// This is the main file of the draw abstraction which actually deals with drawing.
// It is an efficient, portable, functional approach to pixel transformations without the need for hardware acceleration.
// That being said, through overloading would is possible to get this design hardware accelerated for whatever reason.
//
// I ported this style of graphics abstraction to various languages, originally written in Swift it has
// ports to Rust and Kotlin, but besides Swift and to some extent Rust most languages are unable to fully
// express the complex type relationships seen here, especially in an efficient way.
//
// C++ can't really express this well (efficiently) with it's eagerly virtual type system, but abusive techniques
// like SFINAE (through enable_if) or concepts in newer versions of C++ can get the job done.
// The syntax can also compose somewhat decently by... adapting... the idea of std::ranges adapters.
//
// Do NOT inherit from the dyn namespace directly because C++ compilers will NOT devirtualize a thing, negating the benefits
// of using SFINAE/concepts for this in the first place.
// Instead, the convention is to implement existential containers separately, through an instance.dyn() call.
//
// There are some laws associated with the protocols of this library.
// Understanding the simple ideas is important because these types are a lot more fault-tolerant than conventional collections.
//
// The main protocols are:
//
// - Drawable, an infinite plane of pixels which one can get.
//     Usually not used in isolation, this is the core idea of the system. An infinite plane.
//
// - SizedDrawable, a refinement of Drawable bundling it with a width and a height.
//     Most commonly used, this represents slices of infinite planes. Crucial for actually evaluating (rendering) them.
//
// - MutableDrawable, a refinement of Drawable allowing pixels to be set.
//     Quite self explanatory, an infinite plane which can be mutated (almost always combined with a Sized requirement).
//
// - PrimitiveDrawable, a refinement of SizedDrawable which can losslessly be flattened into from another one.
//     This represents primitives, actual concrete roots of a drawable expression, like the Image or InfiniteImage types.
//
// Drawables are structurally equal, implementations of equality and hashing must take into account
// the exact value of every single pixel. Optimizations are allowed, like two red rectangles of the same size
// being equal just by comparing those parameters rather than computing them for each pixel, but this
// MUST preserve the structure. Specifically:
// (a == b) == (flatten(a) == flatten(b))
//
// Drawables are safe to index out of bounds. They can choose how to handle this but they are all a description
// of infinite space with pixel precision. Sized drawables are merely views into an infinite world.
// This has important usability consequences when composing drawables, greatly increasing their expressive power
// For example one can shift a sized slice or resize it. This fault-tolerance is very convenient.
//
// Previously, Drawable was the name of the protocol now known as SizedDrawable, but this added complexity
// turns out to enable a lot of powerful features such as infinite mutable planes implementable through storing
// themselves in chunks.
// In fact, you could think of a stage in a game as a Drawable, it is not inherently possible to draw
// but it is an infinite composition of other drawables one can slice to implement a camera.
//
// HardwareDrawables are a refinement which is meant to be used directly on a hybrid context of some sort
// but they are currently unimplemented simply because it's not worth it at low resolutions like the sonic game,
// and I have absolutely no intention of using C++ for 2d games ever again, it is not a great fit.
#pragma once
#include <primitive>
#include <type_traits>
#include <utility>
#include <algorithm>
#include "Color.hpp"

namespace draw {
    // template <typename Self>
    // concept Drawable = requires(Self const& self, i32 x, i32 y) {
    //     { self.get(x, y) } -> std::same_as<Color>;
    // };

    // template <typename Self>
    // concept MutableDrawable = Drawable<Self> and requires(Self& self, i32 x, i32 y, Color color) {
    //     { self.set(x, y, color) } -> std::same_as<void>;
    // };

    // template <typename Self>
    // concept SizedDrawable = Drawable<Self> and requires(Self const& self) {
    //     { self.width() } -> std::same_as<i32>;
    //     { self.height() } -> std::same_as<i32>;
    // };

    // template <typename Self>
    // concept SizedMutableDrawable = SizedDrawable<Self> and MutableDrawable<Self>;

    // template <typename Self, typename Other>
    // concept PrimitiveDrawable = SizedDrawable<Self> and requires(Other const& other) {
    //     { Self::flatten(other) } -> std::same_as<Self>;
    // };

    template <typename, typename = void> struct Drawable : std::false_type {};
    template <typename, typename = void> struct MutableDrawable : std::false_type {};
    template <typename, typename = void> struct SizedDrawable : std::false_type {};
    template <typename, typename, typename = void> struct PrimitiveDrawable : std::false_type {};

    template <typename Self> struct Drawable<Self, std::enable_if_t<
        std::is_same<decltype(std::declval<Self const&>().get(std::declval<i32>(), std::declval<i32>())), Color>::value
    >> : std::true_type {};

    template <typename Self> struct SizedDrawable<Self, std::enable_if_t<
        Drawable<Self>::value and
        std::is_same<decltype(std::declval<Self const&>().width()), i32>::value and
        std::is_same<decltype(std::declval<Self const&>().height()), i32>::value
    >> : std::true_type {};

    template <typename Self> struct MutableDrawable<Self, std::enable_if_t<
        Drawable<Self>::value and
        std::is_same<decltype(std::declval<Self&>().set(
            std::declval<i32>(), std::declval<i32>(), std::declval<Color>()
        )), void>::value
    >> : std::true_type {};

    template <typename Self, typename From> struct PrimitiveDrawable<Self, From, std::enable_if_t<
        Drawable<Self>::value and SizedDrawable<From>::value and
        std::is_same<decltype(Self::flatten(std::declval<From const&>())), Self>::value
    >> : std::true_type {};
}

namespace draw::dyn {
    /// Dynamically dispatched drawable supertype.
    struct Drawable {
        virtual auto get(i32 x, i32 y) const -> Color = 0;
    };

    /// Dynamically dispatched mutable drawable supertype.
    struct MutableDrawable : Drawable {
        virtual void set(i32 x, i32 y, Color color) = 0;
    };

    /// Dynamically dispatched sized drawable supertype.
    struct SizedDrawable : Drawable {
        virtual auto width() const -> i32 = 0;
        virtual auto height() const -> i32 = 0;
    };

    /// Dynamically dispatched sized mutable drawable supertype.
    struct SizedMutableDrawable : SizedDrawable, MutableDrawable {};
}

/// Performs forwarding adapter composition. Based on the design of std::ranges.
template <typename Self, typename Adapt>
auto operator|(Self&& self, Adapt&& adapt) -> decltype(std::forward<Adapt>(adapt)(std::forward<Self>(self))) {
    return std::forward<Adapt>(adapt)(std::forward<Self>(self));
}

/// When sized, there is a universal, least optimized fallback equality definition available between all sized drawables.
///
/// Comparing infinite drawables is often possible through various tricks based on the math behind
/// its infinite size, except for some truly infinite planes like an InfiniteImage (type not implemented in the C++ version).
template <typename L, typename R, std::enable_if_t<draw::SizedDrawable<L>::value and draw::SizedDrawable<R>::value> = 0>
auto operator==(L const& lhs, R const& rhs) -> bool {
    if (lhs.width() != rhs.width() or lhs.height() != rhs.height()) return false;
    for (i32 x = 0; x < lhs.width(); x += 1) {
        for (i32 y = 0; y < lhs.height(); y += 1) {
            if (lhs.get(x, y) != rhs.get(x, y)) return false;
        }
    }
    return true;
}

/// Older C++ versions do not derive this from equality itself yet.
template <typename L, typename R, std::enable_if_t<draw::SizedDrawable<L>::value and draw::SizedDrawable<R>::value> = 0>
auto operator!=(L const& lhs, R const& rhs) -> bool {
    return !(lhs == rhs);
}

// Flattening ----------------------------------------------------------------------------------------------------------
namespace draw {
    namespace adapt {
        template <typename T> struct Flatten final {
            template <typename U> T operator()(U const& self) const {
                static_assert(SizedDrawable<U>::value);
                static_assert(PrimitiveDrawable<T, U>::value, "Only primitives can be flattened into");

                return T::flatten(self);
            }
        };
    }

    template <typename T> adapt::Flatten<T> flatten() {
        return adapt::Flatten<T> {};
    }
}

// Mutation ------------------------------------------------------------------------------------------------------------
namespace draw {
    namespace adapt {
        struct Clear final {
            Color color;

            template <typename T> T& operator()(T& self) const {
                static_assert(SizedDrawable<T>::value and MutableDrawable<T>::value);

                for (i32 x = 0; x < self.width(); x += 1) {
                    for (i32 y = 0; y < self.height(); y += 1) {
                        self.set(x, y, color);
                    }
                }
                return self;
            }
        };

        template <typename Blend> struct Pixel final {
            i32 x, y;
            Color color;
            Blend blend_mode;

            Pixel(i32 x, i32 y, Color color, Blend blend_mode)
                : x(x), y(y), color(color), blend_mode(blend_mode) {}

            template <typename T> T& operator()(T& self) const {
                static_assert(SizedDrawable<T>::value and MutableDrawable<T>::value);

                self.set(x, y, color.blend_over(self.get(x, y), blend_mode));
                return self;
            }
        };

        template <typename D, typename Blend> struct Draw final {
            D const& drawable;
            i32 x, y;
            Blend blend_mode;

            Draw(D const& drawable, i32 x, i32 y, Blend blend_mode)
                : drawable(drawable), x(x), y(y), blend_mode(blend_mode) {}

            template <typename T> T& operator()(T& self) const {
                static_assert(SizedDrawable<D>::value);
                static_assert(SizedDrawable<T>::value and MutableDrawable<T>::value);

                const auto width = drawable.width();
                const auto height = drawable.height();

                for (i32 x = 0; x < width; x += 1) {
                    for (i32 y = 0; y < height; y += 1) {
                        self.set(
                            x + this->x, y + this->y,
                            drawable.get(x, y).blend_over(self.get(x + this->x, y + this->y), blend_mode)
                        );
                    }
                }

                return self;
            }
        };
    }

    inline adapt::Clear clear(Color color = color::CLEAR) {
        return adapt::Clear { color };
    }

    template <typename Blend> adapt::Pixel<Blend> pixel(i32 x, i32 y, Color color, Blend blend_mode) {
        return adapt::Pixel { x, y, color, blend_mode };
    }

    inline adapt::Pixel<decltype(&blend::binary)> pixel(i32 x, i32 y, Color color) {
        return adapt::Pixel { x, y, color, blend::binary };
    }

    template <typename D, typename Blend> adapt::Draw<D, Blend> draw(D const& drawable, i32 x, i32 y, Blend blend_mode) {
        return adapt::Draw { drawable, x, y, blend_mode };
    }

    template <typename D> adapt::Draw<D, decltype(&blend::binary)> draw(D const& drawable, i32 x, i32 y) {
        return adapt::Draw { drawable, x, y, blend::binary };
    }
}

// Transformation ------------------------------------------------------------------------------------------------------
namespace draw {
    template <typename T> class DrawableSlice final {
        static_assert(Drawable<T>::value);

        T& inner;
        i32 x, y, w, h;

      public:
        constexpr explicit DrawableSlice(T& inner, i32 x, i32 y, i32 width, i32 height)
            : inner(inner), x(x), y(y), w(width), h(height) {}

        constexpr auto get(i32 x, i32 y) const -> Color {
            return inner.get(this->x + x, this->y + y);
        }

        constexpr void set(i32 x, i32 y, Color color) {
            inner.set(this->x + x, this->y + y, color);
        }

        constexpr auto width() const -> i32 {
            return w;
        }

        constexpr auto height() const -> i32 {
            return h;
        }

        constexpr auto resize_left(i32 offset) const -> DrawableSlice {
            return DrawableSlice { inner, x - offset, y, std::max(0, w + offset), h };
        }

        constexpr auto resize_right(i32 offset) const -> DrawableSlice {
            return DrawableSlice { inner, x, y, std::max(0, w + offset), h };
        }

        constexpr auto resize_top(i32 offset) const -> DrawableSlice {
            return DrawableSlice { inner, x, y - offset, w, std::max(0, h + offset) };
        }

        constexpr auto resize_bottom(i32 offset) const -> DrawableSlice {
            return DrawableSlice { inner, x, y, w, std::max(0, h + offset) };
        }

        constexpr auto resize_horizontal(i32 offset) const -> DrawableSlice {
            return DrawableSlice { inner, x - offset, y, std::max(0, w + offset * 2), h };
        }

        constexpr auto resize_vertical(i32 offset) const -> DrawableSlice {
            return DrawableSlice { inner, x, y - offset, w, std::max(0, h + offset * 2) };
        }

        constexpr auto shift(i32 off_x, i32 off_y) const -> DrawableSlice {
            return DrawableSlice { inner, x + off_x, y + off_y, w, h };
        }
    };

    template <typename T> struct DrawableGrid final {
        static_assert(Drawable<T>::value);

        T& inner;
        i32 item_width, item_height;
      public:
        constexpr explicit DrawableGrid(T& inner, i32 item_width, i32 item_height)
            : inner(inner), item_width(item_width), item_height(item_height) {}

        constexpr auto tile(i32 x, i32 y) -> DrawableSlice<T> {
            return DrawableSlice(inner, x * item_width, y * item_height, item_width, item_height);
        }
    };

    namespace adapt {
        struct Slice final {
            i32 x, y, width, height;

            template <typename T> auto operator()(T& inner) const -> DrawableSlice<T> {
                static_assert(Drawable<T>::value);
                return DrawableSlice<T>(inner, x, y, width, height);
            }

            template <typename T> auto operator()(T const& inner) const -> DrawableSlice<const T> {
                static_assert(Drawable<T>::value);
                return DrawableSlice<const T>(inner, x, y, width, height);
            }
        };

        struct Grid final {
            i32 item_width, item_height;

            template <typename T> auto operator()(T& inner) const -> DrawableGrid<T> {
                static_assert(Drawable<T>::value);
                return DrawableGrid<T>(inner, item_width, item_height);
            }

            template <typename T> auto operator()(T const& inner) const -> DrawableGrid<const T> {
                static_assert(Drawable<T>::value);
                return DrawableGrid<const T>(inner, item_width, item_height);
            }
        };

        struct Shift final {
            i32 x, y;

            template <typename T> auto operator()(T& inner) const -> DrawableSlice<T> {
                static_assert(SizedDrawable<T>::value);
                return DrawableSlice<T>(inner, x, y, inner.width(), inner.height());
            }
        };

        struct AsSlice final {
            template <typename T> auto operator()(T& inner) const -> DrawableSlice<T> {
                static_assert(SizedDrawable<T>::value);
                return DrawableSlice<T>(inner, 0, 0, inner.width(), inner.height());
            }

            template <typename T> auto operator()(T const& inner) const -> DrawableSlice<const T> {
                static_assert(SizedDrawable<T>::value);
                return DrawableSlice<const T>(inner, 0, 0, inner.width(), inner.height());
            }
        };
    }

    inline adapt::Slice slice(i32 x, i32 y, i32 width, i32 height) {
        return adapt::Slice { x, y, width, height };
    }

    inline adapt::Grid grid(i32 item_width, i32 item_height) {
        return adapt::Grid { item_width, item_height };
    }

    inline adapt::Shift shift(i32 x, i32 y) {
        return adapt::Shift { x, y };
    }

    inline adapt::AsSlice as_slice() {
        return adapt::AsSlice {};
    }
}

// Mapping -------------------------------------------------------------------------------------------------------------
namespace draw {
    namespace adapt {

    }
}

// Conditionals --------------------------------------------------------------------------------------------------------
namespace draw {
    template <typename Left, typename Right> class EitherDrawable final {
        static_assert(Drawable<Left>::value and Drawable<Right>::value);

        enum class Case { L, R } tag;
        bool is_alive { true };

        union {
            std::reference_wrapper<const Left> left;
            std::reference_wrapper<const Right> right;
        };

      public:
        EitherDrawable(Left const& value) : tag(Case::L) {
            left = value;
        }
        EitherDrawable(Right const& value) : tag(Case::R) {
            right = value;
        }

        auto width() const -> i32 {
            static_assert(SizedDrawable<Left>::value and SizedDrawable<Right>::value);
            switch (tag) {
                case Case::L: return left.get().width();
                case Case::R: return right.get().width();
            }
        }

        auto height() const -> i32 {
            static_assert(SizedDrawable<Left>::value and SizedDrawable<Right>::value);
            switch (tag) {
                case Case::L: return left.get().height();
                case Case::R: return right.get().height();
            }
        }

        auto get(i32 x, i32 y) const -> Color {
            switch (tag) {
                case Case::L: return left.get().get(x, y);
                case Case::R: return right.get().get(x, y);
            }
        }
    };

    namespace adapt {
        template <typename Adapter> struct ApplyIf final {
            bool cond;
            Adapter const& fn;

            ApplyIf(bool cond, Adapter const& fn) : cond(cond), fn(fn) {}

            template <typename T> auto operator()(T const& inner) const -> EitherDrawable<T, decltype(fn(inner))> {
                if (not cond) {
                    return EitherDrawable<T, decltype(fn(inner))>(inner);
                } else {
                    return EitherDrawable<T, decltype(fn(inner))>(fn(inner));
                }
            }
        };
    }

    template <typename Adapter> adapt::ApplyIf<Adapter> apply_if(bool cond, Adapter const& fn) {
        return adapt::ApplyIf<Adapter> { cond, fn };
    }
}

// Layout --------------------------------------------------------------------------------------------------------------
namespace draw {
    enum class MirrorAxis : u8 { X, Y };

    template <const MirrorAxis AXIS, typename T> struct MirroredDrawable final {
        static_assert(SizedDrawable<T>::value);

        T const& inner;

        explicit MirroredDrawable(T const& inner) : inner(inner) {}

        auto width() const -> i32 {
            return inner.width();
        }

        auto height() const -> i32 {
            return inner.height();
        }

        auto get(i32 x, i32 y) const -> Color {
            if constexpr (AXIS == MirrorAxis::X) {
                return inner.get(width() - 1 - x, y);
            } else {
                return inner.get(x, height() - 1 - y);
            }
        }
    };

    namespace adapt {
        template <const MirrorAxis AXIS> struct Mirror final {
            template <typename T> auto operator()(T const& inner) const -> MirroredDrawable<AXIS, T> {
                static_assert(SizedDrawable<T>::value);
                return MirroredDrawable<AXIS, T>(inner);
            }
        };
    }

    inline adapt::Mirror<MirrorAxis::X> mirror_x() {
        return adapt::Mirror<MirrorAxis::X> {};
    }

    inline adapt::Mirror<MirrorAxis::Y> mirror_y() {
        return adapt::Mirror<MirrorAxis::Y> {};
    }
}
