// Created by Lua (TeamPuzel) on May 26th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <primitive>
#include <math>
#include <rt>
#include <type_traits>

namespace sonic {
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
    /// The data of an entity is a match of the state described in the Sonic Physics Guide.
    class Object {
      public:
        math::point<fixed> position;
        math::point<fixed> velocity;
        u16 width_radius;
        u16 height_radius;
        fixed ground_velocity;
        math::angle ground_angle;

        /// Called each tick at 60hz.
        virtual void update(rt::Input const& input, Stage& scene) {}

        struct Sprite final { i32 x, y, w, h; };

        virtual auto sprite() const -> Sprite {
            return Sprite { 0, 0, 0, 0 };
        };

        virtual auto sprite_offset() const -> math::point<i32> {
            return math::point { 0, 0 };
        }

        virtual ~Object() noexcept {}

        auto tile_pos() const -> math::point<i32> {
            return math::point { i32(position.x) / 16, i32(position.y) / 16 };
        }

        auto pixel_pos() const -> math::point<i32> {
            return math::point { i32(position.x), i32(position.y) };
        }

        /// Called when debug drawing is enabled, meant to visualise collision etc.
        virtual void debug_draw(draw::DrawableSlice<draw::Image>& target) const {}
    };

    /// A game object which can be serialized.
    ///
    /// Any object loaded from the map file has to be serializable.
    /// If an object is only used dynamically it does not need to be.
    ///
    /// Map loadable objects must implement this trait to be added into the load registry.
    ///
    /// trait SerializableObject {
    ///     static deserialize(BinaryReader&) -> Self;
    ///     static serialize(Self const&, BinaryWriter&); // this one is not needed at the moment.
    /// }
    template <typename, typename = void> struct SerializableObject : std::false_type {};
    template <typename Self> struct SerializableObject<Self, std::enable_if_t<
        std::is_same<decltype(Self::deserialize(std::declval<rt::BinaryReader&>())), Self>::value
    >> : std::true_type {};

    using Deserializer = auto (rt::BinaryReader&) -> box<Object>;

    /// A registry is a functor mapping object class names from the file format onto their deserializer.
    ///
    /// C++ is not an object oriented language so one can't return the class as C++ has no classes, only
    /// instances. For this reason the function pointer itself has to be returned.
    ///
    /// trait ObjectRegistry = Fn(cstring) -> fn*(BinaryReader&) -> Object*;
    template <typename, typename = void> struct ObjectRegistry : std::false_type {};
    template <typename Self> struct ObjectRegistry<Self, std::enable_if_t<
        std::is_same<
            decltype(std::declval<Self const&>()(std::declval<std::string_view>())),
            Deserializer*
        >::value
    >> : std::true_type {};

    /// The player entity representing Sonic himself.
    class Sonic final : public Object {
      public:
        void update(rt::Input const& input, Stage& stage) override {
            if (input.key_held(rt::Key::Up)) position.y -= 1;
            if (input.key_held(rt::Key::Down)) position.y += 1;
            if (input.key_held(rt::Key::Left)) position.x -= 1;
            if (input.key_held(rt::Key::Right)) position.x += 1;
        }

        auto sprite() const -> Sprite override {
            return Sprite { 0, 6, 64, 64 };
        }

        auto sprite_offset() const -> math::point<i32> override {
            return math::point { -32, -52 };
        }

        static auto deserialize(rt::BinaryReader& reader) -> box<Object> {
            return box<Sonic>::make();
        }
    };

    /// A special object which
    class LayerSwitch final : public Object {};

    /// Collectable rings.
    class Ring final : public Object {
      public:
        static auto deserialize(rt::BinaryReader& reader) -> box<Object> {
            return box<Ring>::make();
        }
    };

    /// Spikes used throughout the level.
    class Spike final : public Object {};

    /// A moving platform.
    class Platform final : public Object {};

    /// A cliff which collapses when touched.
    class CollapsingCliff final : public Object {};

    /// A breakable monitor providing power-ups.
    class Monitor final : public Object {};

    /// A goal sign present at the end of any non-final act.
    class Goal final : public Object {};

    /// A checkpoint gate.
    class Checkpoint final : public Object {};

    /// A spring launching the player in a direction.
    class Spring final : public Object {};

    /// A hostile entity launching up from water to try and bite the player.
    class Chopper final : public Object {};

    /// A basic hostile entity moving about the ground.
    class MotoBug final : public Object {};

    /// A hostile crab walking about and launching projectiles sometimes.
    class Crabmeat final : public Object {};

    /// A flying hostile entity and launching projectiles at the player.
    class BuzzBomber final : public Object {};

    class BlueNewtron final : public Object {};

    class GreenNewtron final : public Object {};

    class Animal final : public Object {};
}
