// Created by Lua (TeamPuzel) on August 11th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <sonic>

namespace sonic {
    /// The player entity representing Sonic himself.
    class Sonic final : public Object, public Codable<Sonic> {
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
                return Mode::Floor; // Unreachable backup.
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
                return Mode::Floor; // Unreachable backup.
            }
        }

        auto general_mode() const noexcept -> Mode {
            return ground_sensor_mode(); // Let's just reuse these elsewhere.
        }

        void snap_angle() noexcept {
            if (ground_angle >= 316 and ground_angle <= 44) {
                ground_angle = 0;
            } else if (ground_angle >= 45 and ground_angle <= 135) {
                ground_angle = 90;
            } else if (ground_angle >= 136 and ground_angle <= 224) {
                ground_angle = 180;
            } else if (ground_angle >= 225 and ground_angle <= 315) {
                ground_angle = 270;
            } else {
                ground_angle = 0; // Unreachable backup.
            }
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

        void debug_draw(std::stringstream& out, draw::Slice<Ref<Image>> target, Stage const& stage) const noexcept override;
    };
}
