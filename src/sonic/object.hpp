// Created by Lua (TeamPuzel) on May 26th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <primitive>
#include <math>
#include <rt>
#include <type_traits>

namespace sonic {
    using draw::Image;
    using draw::Ref;
    using draw::Color;
    using math::point;
    using math::angle;

    class Stage;

    /// A simple animation system which scrolls through and loops in a range.
    ///
    /// The speed for the next iteration can be adjusted as the current iteration plays which can
    /// accurately recreate the behavior of animations from the classic sonic games.
    template <typename T, const T DEFAULT = T(0)> class Animator final {
        static_assert(std::is_enum<T>::value);
        T current { DEFAULT };

        /// The current frame of the animation.
        u32 frame { 0 };
        /// A counter counting down frames.
        u32 counter { 0 };
        /// How many frames are in this animation.
        u32 count { 1 };
        /// The frame the loop jumps back to.
        u32 loop { 0 };
        /// How many extra frames does it take to move on to the next frame.
        u32 speed { 0 };

      public:
        constexpr Animator() noexcept {}

        constexpr auto which() const noexcept -> T {
            return current;
        }

        constexpr auto is(T anim) const noexcept -> bool {
            return anim == current;
        }

        constexpr auto at() const noexcept -> u32 {
            return frame;
        }

        constexpr auto play(T anim, u32 count = 1, u32 speed = 0, u32 loop = 0) noexcept -> bool {
            if (anim == current) return false;

            this->current = anim;
            this->frame = 0;
            this->counter = speed;
            this->count = std::max(1u, count);
            this->loop = loop;
            this->speed = speed;

            return true;
        }

        constexpr void set_speed(u32 step) noexcept {
            this->speed = step;
        }

        /// Step through the animation.
        constexpr void update() noexcept {
            if (counter == 0) {
                frame += 1;
                counter = speed;
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
        point<fixed> position;
        point<fixed> speed;
        fixed ground_speed;
        angle ground_angle;

        /// Called each tick at 60hz.
        virtual void update(rt::Input const& input, Stage& stage) noexcept {}

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

        /// Called when debug drawing is enabled, meant to visualise collision etc.
        virtual void debug_draw(draw::Slice<Ref<Image>> target, Stage const& stage) const noexcept {}
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

    using Deserializer = auto (rt::BinaryReader&) -> Box<Object>;

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
        enum class State : u8 {
            Normal,
            Rolling,
            Airborne,
        } state { State::Normal };

        fixed spinrev { 0 };
        u16 control_lock { 0 };

        enum class Animation {
            Standing,
            Walking,
            Running,
            Rolling,
            Skidding,
        };

        mutable Animator<Animation> animator {};
        mutable bool mirror_x { false };
        mutable i32 anim_x { 0 }, anim_y { 6 };

        enum class Mode : u8 {
            Floor,
            RightWall,
            Ceiling,
            LeftWall,
        };

        auto ground_sensor_mode() const noexcept -> Mode {
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

        auto push_sensor_mode() const noexcept -> Mode {
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

        auto general_mode() const noexcept -> Mode {
            return ground_sensor_mode(); // Let's just reuse these elsewhere.
        }

        auto is_half_steep() const noexcept -> bool {
            const auto angle = (u32) ground_angle % 90;
            return angle > 45;
        }

        [[clang::always_inline]] [[gnu::pure]]
        auto width_radius() const noexcept -> i32 {
            switch (state) {
                case State::Normal:   return 9;
                case State::Rolling:  return 7;
                case State::Airborne: return 7;
            }
        }

        [[clang::always_inline]] [[gnu::pure]]
        auto height_radius() const noexcept -> i32 {
            switch (state) {
                case State::Normal:   return 19;
                case State::Rolling:  return 14;
                case State::Airborne: return 14;
            }
        }

        static constexpr fixed JUMP_FORCE              = fixed(6, 128);
        static constexpr fixed GRAVITY_FORCE           = fixed(0, 56 );
        static constexpr fixed ACCELERATION_SPEED      = fixed(0, 12 );
        static constexpr fixed DECELERATION_SPEED      = fixed(0, 128);
        static constexpr fixed FRICTION_SPEED          = fixed(0, 12 );
        static constexpr fixed TOP_SPEED               = fixed(6, 0  );
        static constexpr fixed ROLL_FRICTION_SPEED     = fixed(0, 6  );
        static constexpr fixed ROLL_DECELERATION_SPEED = fixed(0, 32 );
        static constexpr fixed AIR_ACCELERATION_SPEED  = fixed(0, 24 );
        static constexpr fixed SLOPE_FACTOR_NORMAL     = fixed(0, 32 );
        static constexpr fixed SLOPE_FACTOR_ROLL_UP    = fixed(0, 20 );
        static constexpr fixed SLOPE_FACTOR_ROLL_DOWN  = fixed(0, 80 );
        static constexpr fixed HURT_X_FORCE            = fixed(2, 0  );
        static constexpr fixed HURT_Y_FORCE            = fixed(4, 0  );
        static constexpr fixed HURT_GRAVITY            = fixed(0, 48 );

        void update(rt::Input const& input, Stage& stage) noexcept override;

        auto sprite(rt::Input const& input) const noexcept -> Sprite override {
            using rt::Key;

            if (ground_speed < 0) mirror_x = true;
            if (ground_speed > 0) mirror_x = false;

            switch (state) {
                case State::Normal: {
                    const fixed abs_speed = math::abs(ground_speed);

                    if ((abs_speed > 4 or animator.is(Animation::Skidding)) and (
                        ground_speed < 0 and input.key_held(Key::Right) and not input.key_held(Key::Left) or
                        ground_speed > 0 and input.key_held(Key::Left) and not input.key_held(Key::Right)
                    )) {
                        if (animator.play(Animation::Skidding, 2, 8)) {
                            anim_x = 6; anim_y = 7;
                        }
                    } else if (abs_speed == 0) {
                        animator.play(Animation::Standing);

                        if (input.key_held(Key::Down)) {
                            anim_x = 6, anim_y = 6;
                        } else if (input.key_held(Key::Up)) {
                            anim_x = 5, anim_y = 6;
                        } else {
                            anim_x = 0, anim_y = 6;
                        }
                    } else if (abs_speed > 0 and abs_speed < TOP_SPEED) {
                        if (animator.play(Animation::Walking, 6)) anim_x = 0, anim_y = 7;
                    } else if (abs_speed >= TOP_SPEED) {
                        if (animator.play(Animation::Running, 4)) anim_x = 0, anim_y = 9;
                    }

                    if (animator.is(Animation::Walking) or animator.is(Animation::Running)) {
                        animator.set_speed((i32) math::floor(std::max<fixed>(0, 8 - abs_speed)));

                        // if (is_half_steep()) {
                        //     anim_y = animator.is(Animation::Walking) ? 8 : 10;
                        // } else {
                        //     anim_y = animator.is(Animation::Walking) ? 7 : 9;
                        // }
                    }

                    animator.update();
                } break;
                case State::Rolling: {
                    animator.play(Animation::Rolling, 4);
                    anim_x = 0, anim_y = 11;

                    if (
                        input.counter() % 3 == 0 and math::abs(ground_speed) >= TOP_SPEED or
                        input.counter() % 5 == 0
                    ) {
                        anim_x = 4 - (i32) animator.at();
                    } else {
                        anim_x = 0;
                        animator.update();
                    }
                } break;
                case State::Airborne: {

                } break;
            }

            return Sprite { anim_x + (i32) animator.at(), anim_y, 64, 64, mirror_x, false, (u8) ground_sensor_mode() };
        }

        void debug_draw(draw::Slice<Ref<Image>> target, Stage const& stage) const noexcept override;

        static auto deserialize(rt::BinaryReader& reader) -> Box<Object> {
            return Box<Sonic>::make();
        }
    };

    /// A special object which
    class LayerSwitch final : public Object {};

    /// Collectable rings.
    class Ring final : public Object {
      public:
        static auto deserialize(rt::BinaryReader& reader) -> Box<Object> {
            return Box<Ring>::make();
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
