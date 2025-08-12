// Created by Lua (TeamPuzel) on May 26th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <primitive>
#include <math>
#include <rt>
#include <font>
#include <sstream>

namespace sonic {
    using draw::Image;
    using draw::Ref;
    using draw::Color;
    using draw::Text;
    using math::point;
    using math::angle;

    class Stage;

    /// A dynamic game object.
    ///
    /// Anything that isn't part of the tile grid.
    ///
    /// These assume 60Hz updates for multiple reasons:
    /// - 8 bit fixed point arithmetic doesn't work with delta time, I didn't add precision only
    ///   to mess with it using non-deterministic techniques.
    /// - The original games worked like this so it's more accurate.
    /// - It's easy to match updates perfectly evenly to any sane refresh rate (144Hz can go and disappear for all I care).
    /// - Graphics could be interpolated between updates which I might do later. At that point precision
    ///   is less important since it doesn't affect the simulation itself.
    ///
    /// The data of an object is a match of the state described in the Sonic Physics Guide.
    /// You could divide this data up over some overengineered inheritance hierarchy
    /// but I do not want to bother for literally 6 variables, I'll just do what the original game did.
    ///
    /// It is a flat inheritance hierarchy instead as that's generally more readable in C++
    /// because it has no sensible syntactic conveniences for type composition such as
    /// consistent dereferencing syntax. It inherits the "."/"->" nonsense from C.
    class Object {
        friend class Stage;

        /// The class name is used by the stage to relate types to their source libraries.
        /// Local objects retain an empty classname which is how the stage can tell them apart.
        std::string classname;

        auto is_dynobject() const -> bool {
            return not classname.empty();
        }

      public:
        point<fixed> position;
        point<fixed> speed;
        fixed ground_speed;
        angle ground_angle;

        Object() = default;
        Object(Object const&) = delete;
        Object(Object&&) = delete;
        auto operator=(Object const&) -> Object& = delete;
        auto operator=(Object&&) -> Object& = delete;

        /// Called once every tick at 60hz.
        virtual void update(rt::Input const& input, Stage& stage) noexcept {}

        enum class Mode : u8 {
            Floor,
            RightWall,
            Ceiling,
            LeftWall,
        };

        struct Sprite final {
            i32 x { 0 }, y { 0 }, w { 0 }, h { 0 };
            bool mirror_x { false }, mirror_y { false };
            u8 rotation { 0 };
        };

        virtual auto sprite(rt::Input const&) const noexcept -> Sprite {
            return Sprite {};
        };

        virtual ~Object() noexcept {}

        auto tile_pos() const noexcept -> math::point<i32> {
            return math::point { i32(position.x) / 16, i32(position.y) / 16 };
        }

        auto pixel_pos() const noexcept -> math::point<i32> {
            return math::point { i32(position.x), i32(position.y) };
        }

        auto is_underwater() const noexcept -> bool {
            return false;
        }

        /// Called when debug drawing is enabled, meant for visualising collision etc.
        /// The object receives the global debug overlay output and the camera slice to draw into freely.
        virtual void debug_draw(std::stringstream& out, draw::Slice<Ref<Image>> target, Stage const& stage) const noexcept {}
    };

    /// Provides default implementations of the dynamic object interface.
    template <typename Self> struct Codable {
        static auto rebuild(Self const& existing) -> Box<Object> {
            return Box<Self>::make();
        }

        static auto deserialize(rt::BinaryReader& reader) -> Box<Object> {
            return Box<Self>::make();
        }

        void serialize(rt::BinaryWriter& writer) const {
            // TODO: Serialize basics and classname.
        }
    };
}
