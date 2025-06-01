// Created by Lua (TeamPuzel) on May 29th 2025.
// Copyright (c) 2025 All rights reserved.
// 
// This is the main file of the draw library which actually deals with drawing.
// It is an efficient, functional approach to pixel transformations.
// 
// I ported this style of graphics abstraction to various languages, originally written in Swift it has
// ports to Rust and Kotlin, but besides Swift and to some extent Rust most languages are unable to fully
// express the complex type relationships seen here, especially in an efficient way.
// 
// C++ can't truly express OOP as it is missing protocols, but this can be achieved through abuse
// of SFINAE (enable_if) or concepts in newer versions of C++.
// 
// This is a C++23 port of the abstraction. Because it relies on extending existing types rather
// than inheritance the approach is similar to how std::ranges worked around this. It would be possible
// to hardcode most of the functionality with multiple inheritance, but after testing this in
// compiler explorer even clang is unable to devirtualize that, so it would end up incredibly inefficient.
// For this reason I accept the added complexity of template abuse in order to provide drawables to C++
// with no runtime overhead when not required, however base classes for corresponding vtables do exist
// to allow for covariance in more dynamic contexts.
// 
// Do NOT inherit from these virtual types because C++ will NOT devirtualize them, negating the benefits
// of using SFINAE/concepts for this in the first place.
// Instead, the convention is to implement those separately, through an instance.dyn() call.
// 
// There are some laws associated with the protocols of this library.
// 
// The main protocols are:
// - Drawable, an infinite plane of pixels which one can get.
// - SizedDrawable, a refinement of Drawable with a width and height.
// - MutableDrawable, a refinement of Drawable allowing pixels to be set.
// - SizedMutableDrawable, a convenience equivalent to just constraining to both prior protocols, it is their union.
// - HardwareDrawable (unimplemented), a drawable which can be computed on a GPU or similar accelerator.
// - PrimitiveDrawable, a refinement of SizedDrawable which can losslessly be flattened into from another one.
// 
// Drawables are structurally equal, implementations of equality and hashing must take into account
// the exact value of every single pixel. Optimizations are allowed, like two red rectangles of the same size
// being equal just by comparing those parameters rather than computing them for each pixel, but this
// MUST preserve the structure. Specifically:
// (a == b) == (flatten(a) == flatten(b))
// 
// Previously, Drawable was the name of the protocol now known as SizedDrawable, but this added complexity
// turns out to enable a lot of powerful features such as infinite mutable planes implementable through storing
// themselves in chunks.
// In fact, you could think of a stage in a game as a Drawable, it is not inherently possible to draw
// but it is an infinite composition of other drawables one can slice to implement a camera.
// 
// HardwareDrawables are a refinement which is meant to be used directly on a hybrid context of some sort
// but they are currently unimplemented simply because it's not worth it at low resolutions like the sonic game,
// and I have absolutely no intention of using C++ for 2d games ever again, it is a terrible fit.
#pragma once
#include <core>
#include <concepts>
#include <utility>

#include "Color.hpp"

namespace core::draw {
    template <typename Self>
    concept Drawable = requires(Self const& self, i32 x, i32 y) {
        { self.get(x, y) } -> std::same_as<Color>;
    };

    template <typename Self>
    concept MutableDrawable = Drawable<Self> and requires(Self& self, i32 x, i32 y, Color color) {
        { self.set(x, y, color) } -> std::same_as<void>;
    };

    template <typename Self>
    concept SizedDrawable = Drawable<Self> and requires(Self const& self) {
        { self.width() } -> std::same_as<i32>;
        { self.height() } -> std::same_as<i32>;
    };

    template <typename Self>
    concept SizedMutableDrawable = SizedDrawable<Self> and MutableDrawable<Self>;
    
    template <typename Self, typename Other>
    concept PrimitiveDrawable = SizedDrawable<Self> and requires(Other const& other) {
        { Self::flattening(other) } -> std::same_as<Self>;
    };
}

namespace core::draw::dyn {
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

template <typename Self, typename Adapt>
auto operator|(Self&& self, Adapt&& adapt) -> decltype(std::forward<Adapt>(adapt)(std::forward<Self>(self))) {
    return std::forward<Adapt>(adapt)(std::forward<Self>(self));
}

/// When sized there is a universal, least optimized fallback equality available between all sized drawables.
/// 
/// Comparing infinite drawables is often possible through various tricks based on the math behind
/// its infinite size, except for some truly infinite planes like an InfiniteImage (not implemented in the C++ version).
auto operator==(core::draw::SizedDrawable auto const& lhs, core::draw::SizedDrawable auto const& rhs) -> bool {
    if (lhs.width() != rhs.width() or lhs.height() != rhs.height()) return false;
    for (i32 x = 0; x < lhs.width(); x += 1) {
        for (i32 y = 0; y < lhs.height(); y += 1) {
            if (lhs.get(x, y) != rhs.get(x, y)) return false; 
        }
    }
    return true;
}

// Mutation ------------------------------------------------------------------------------------------------------------
namespace core::draw {
    namespace adapt {
        struct Clear final {
            Color color;
            
            template <SizedMutableDrawable T> T& operator()(T& self) const {
                for (i32 x = 0; x < self.width(); x += 1) {
                    for (i32 y = 0; y < self.height(); y += 1) {
                        self.set(x, y, color);
                    }
                }
                return self;
            }
        };
    }
    
    adapt::Clear clear(Color color = pal::CLEAR) {
        return adapt::Clear { color };
    }
}
 
// Transformation ------------------------------------------------------------------------------------------------------
namespace core::draw {
    template <Drawable T> class DrawableSlice final {
        T inner;
        i32 x, y, w, h;

      public:
        constexpr explicit DrawableSlice(T inner, i32 x, i32 y, i32 width, i32 height)
            : inner(std::move(inner)), x(x), y(y), w(width), h(height) {}

        constexpr auto get(i32 x, i32 y) const -> Color {
            return inner.get(this->x + x, this->y + y);
        }

        constexpr auto width() const -> i32 {
            return w;
        }

        constexpr auto height() const -> i32 {
            return h;
        }
    };

    namespace adapt {
        struct Slice final {
            i32 x, y, width, height;

            template <Drawable T> auto operator()(T&& inner) const -> DrawableSlice<std::decay_t<T>> {
                return DrawableSlice(std::forward<T>, x, y, width, height);
            }
        };
    }

    adapt::Slice slice(i32 x, i32 y, i32 width, i32 height) {
        return adapt::Slice { x, y, width, height };
    }
}
