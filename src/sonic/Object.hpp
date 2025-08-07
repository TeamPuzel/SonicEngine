// Created by Lua (TeamPuzel) on May 26th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <primitive>
#include <math>
#include <rt>
#include <type_traits>

namespace sonic {
    class Stage;

    /// A simple animation system which scrolls through and loops in a range.
    /// Everything is public, this type is designed to allow freely messing with the data as it plays.
    /// This abstraction is just here to simplify iterating through a little.
    struct Animator final {
        /// The current frame of the animation.
        u32 frame { 0 };
        /// A counter counting down frames.
        u32 counter { 0 };
        /// How many frames are in this animation.
        u32 count { 1 };
        /// The frame the loop jumps back to.
        u32 loop { 0 };
        /// How many extra frames does it take to move on to the next frame.
        u32 step { 0 };

        constexpr void reset() noexcept {
            *this = Animator {};
        }

        /// Step through the animation.
        constexpr void update() noexcept {
            if (counter == 0) {
                frame += 1;
                counter = step;
            } else {
                counter -= 1;
            }

            if (frame >= count) frame = loop;
        }
    };

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
      public:
        math::point<fixed> position;
        math::point<fixed> speed;
        u16 width_radius;
        u16 height_radius;
        fixed ground_speed;
        math::angle ground_angle;

        /// Called each tick at 60hz.
        virtual void update(rt::Input const& input, Stage& scene) {}

        struct Sprite final {
            i32 x { 0 }, y { 0 }, w { 0 }, h { 0 };
            bool mirror_x { false }, mirror_y { false };
            u8 rotate { 0 };
        };

        virtual auto sprite() const -> Sprite {
            return Sprite {};
        };

        virtual ~Object() noexcept {}

        auto tile_pos() const -> math::point<i32> {
            return math::point { i32(position.x) / 16, i32(position.y) / 16 };
        }

        auto pixel_pos() const -> math::point<i32> {
            return math::point { i32(position.x), i32(position.y) };
        }

        auto is_underwater() const -> bool {
            return false;
        }

        /// Called when debug drawing is enabled, meant to visualise collision etc.
        virtual void debug_draw(draw::DrawableSlice<draw::Ref<draw::Image>>& target) const {}
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
        enum class State : u8 {
            Normal,
            Rolling,
            Airborne,
        } state { State::Normal };

        fixed spinrev { 0 };
        u16 control_lock { 0 };
        mutable Animator animator {};
        mutable bool mirror_x { false };
        mutable i32 anim_x { 0 }, anim_y { 6 };

        mutable enum class Animation {
            Standing,
            Walking,
            Running,
        } animation;

        enum class Mode : u8 {
            Floor,
            RightWall,
            Ceiling,
            LeftWall,
        };

        auto ground_sensor_mode() const -> Mode {
            if (ground_angle >= 315 and ground_angle <= 45) {
                return Mode::Floor;
            } else if (ground_angle >= 46 and ground_angle <= 134) {
                return Mode::RightWall;
            } else if (ground_angle >= 135 and ground_angle <= 225) {
                return Mode::Ceiling;
            } else if (ground_angle >= 226 and ground_angle <= 314) {
                return Mode::LeftWall;
            } else {
                return Mode::Floor; // Unreachable backup, std::unreachable wasn't standard prior to C++23.
            }
        }

        auto push_sensor_mode() const -> Mode {
            if (ground_angle >= 316 and ground_angle <= 44) {
                return Mode::Floor;
            } else if (ground_angle >= 45 and ground_angle <= 135) {
                return Mode::RightWall;
            } else if (ground_angle >= 136 and ground_angle <= 224) {
                return Mode::Ceiling;
            } else if (ground_angle >= 225 and ground_angle <= 315) {
                return Mode::LeftWall;
            } else {
                return Mode::Floor; // Unreachable backup, std::unreachable wasn't standard prior to C++23.
            }
        }

        auto general_mode() const -> Mode {
            return ground_sensor_mode(); // Let's just reuse these elsewhere.
        }

      public:
        void update(rt::Input const& input, Stage& stage) override;

        auto sprite() const -> Sprite override {
            if (ground_speed < 0) mirror_x = true;
            if (ground_speed > 0) mirror_x = false;

            const fixed abs_speed = math::abs(ground_speed);
            if (abs_speed == 0 and animation != Animation::Standing) {
                animation = Animation::Standing;
                animator.reset();

                anim_x = 0; anim_y = 6;
            } else if (abs_speed > 0 and abs_speed < 6 and animation != Animation::Walking) {
                animation = Animation::Walking;
                animator.reset();
                animator.count = 6;

                anim_x = 0; anim_y = 7;
            } else if (abs_speed >= 6 and animation != Animation::Running) {
                animation = Animation::Running;
                animator.reset();
                animator.count = 4;

                anim_x = 0; anim_y = 9;
            }

            if (animation == Animation::Walking or animation == Animation::Running)
                animator.step = (i32) math::floor(std::max<fixed>(0, 8 - math::abs(ground_speed)));

            animator.update();

            return Sprite { anim_x + (i32) animator.frame, anim_y, 64, 64, mirror_x, false, 0 };
        }

        static auto deserialize(rt::BinaryReader& reader) -> box<Object> {
            return box<Sonic>::make();
        }

        static constexpr fixed JUMP_FORCE = fixed(6, 128);
        static constexpr fixed GRAVITY_FORCE = fixed(0, 56);
        static constexpr fixed ACCELERATION_SPEED = fixed(0, 12);
        static constexpr fixed DECELERATION_SPEED = fixed(0, 128);
        static constexpr fixed FRICTION_SPEED = fixed(0, 12);
        static constexpr fixed TOP_SPEED = fixed(6, 0);
        static constexpr fixed ROLL_FRICTION_SPEED = fixed(0, 6);
        static constexpr fixed ROLL_DECELERATION_SPEED = fixed(0, 32);
        static constexpr fixed AIR_ACCELERATION_SPEED = fixed(0, 24);
        static constexpr fixed SLOPE_FACTOR_NORMAL = fixed(0, 32);
        static constexpr fixed SLOPE_FACTOR_ROLL_UP = fixed(0, 20);
        static constexpr fixed SLOPE_FACTOR_ROLL_DOWN = fixed(0, 80);
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
