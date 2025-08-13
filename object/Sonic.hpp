// Created by Lua (TeamPuzel) on August 11th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <sonic>
#include <sstream>

namespace sonic {
    /// The player entity representing Sonic himself.
    class Sonic final : public Object, public Codable<Sonic> {
      public:
        enum class State : u8 {
            Normal,
            Rolling,
            Airborne,
        } state { State::Normal };

        u32 score { 0 };
        u32 timer { 0 };
        u32 rings { 0 };
        u32 lives { 3 };

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

        void update(rt::Input const& input, Stage& stage) noexcept override {
            using rt::Key;
            using math::angle;

            // Special case overriding any player update logic at all with debug flight.
            if (stage.movement_debug) {
                if (input.key_held(Key::Up))    position.y -= 10;
                if (input.key_held(Key::Down))  position.y += 10;
                if (input.key_held(Key::Left))  position.x -= 10;
                if (input.key_held(Key::Right)) position.x += 10;
                return;
            }

            // This is a faithful implementation of classic 16 bit sonic physics based directly on the Sonic Physics Guide.
            // The comments come from the guide and provide a clear outline of what's going on.
            //
            // The physics have three distinct modes, you can be aligned to the ground, in the air or rolling.
            switch (state) {
                case State::Normal: { // "Normal" means any time the Player is not airborne or rolling.
                    // Check for special animations that prevent control (such as balancing).
                    if (false) {
                        // TODO: Low priority.
                    }

                    // Check for starting a spindash while crouched.
                    if (false) {
                        // N/A as this was not a mechanic in the first game yet.
                        // I might add it later as a bonus since modern ports of the game do actually
                        // implement it, but in the end the original game is designed without this ability in mind
                        // and it trivializes some challenges.
                    }

                    // Adjust Ground Speed based on current Ground Angle (Slope Factor).
                    const fixed slope_factor = SLOPE_FACTOR_NORMAL;
                    ground_speed -= slope_factor * math::sin(ground_angle);

                    // Check for starting a jump.
                    if (input.key_pressed(Key::X)) speed.y -= JUMP_FORCE;

                    {
                        const auto left = input.key_held(Key::Left), right = input.key_held(Key::Right);

                        // Update Ground Speed based on directional input and apply friction/deceleration and the speed cap.
                        if (left or right) {
                            if (left and not right) {
                                if (ground_speed > 0) {
                                    ground_speed -= DECELERATION_SPEED;
                                } else if (ground_speed > -TOP_SPEED) {
                                    ground_speed -= ACCELERATION_SPEED;
                                    if (ground_speed <= -TOP_SPEED) ground_speed = -TOP_SPEED;
                                }
                            }

                            if (right and not left) {
                                if (ground_speed < 0) {
                                    ground_speed += DECELERATION_SPEED;
                                } else if (ground_speed < TOP_SPEED) {
                                    ground_speed += ACCELERATION_SPEED;
                                    if (ground_speed >= TOP_SPEED) ground_speed = TOP_SPEED;
                                }
                            }
                        }

                        if (left == right) {
                            ground_speed -= std::min(math::abs(ground_speed), FRICTION_SPEED) * math::sign(ground_speed);
                        }
                    }

                    // Check for starting crouching, balancing on ledges, etc.

                    // Push Sensor collision occurs.
                    // Which sensors are used varies based on the the sensor activation.
                    // This occurs before the Player's position physically moves, meaning they might not actually be touching
                    // the wall yet, the game accounts for this by adding the Player's X Speed and Y Speed to the sensor's position.
                    const auto push_mode = push_sensor_mode();

                    // Check for starting a roll.
                    if ((input.key_held(Key::Down)) and ground_speed >= fixed(0, 128)) {
                        state = State::Rolling;
                    }

                    // Move the Player object
                    // Calculate X Speed and Y Speed from Ground Speed and Ground Angle.
                    // Updates X Position and Y Position based on X Speed and Y Speed.
                    speed.x = ground_speed *  math::cos(ground_angle);
                    speed.y = ground_speed * -math::sin(ground_angle);
                    position += speed;

                    // Grounded Ground Sensor collision occurs.
                    // Updates the Player's Ground Angle.
                    // Align the Player to surface of terrain or become airborne if none found.
                    const auto ground_mode = ground_sensor_mode();

                    const auto sensor_a = stage.sense(this, -width_radius(), height_radius(), Stage::SensorDirection::Down, ground_mode);
                    const auto sensor_b = stage.sense(this,  width_radius(), height_radius(), Stage::SensorDirection::Down, ground_mode);

                    const auto sensor = sensor_b.distance < sensor_a.distance
                        ? sensor_b
                        : sensor_a;

                    // Take care to align with the ground respecting the mode.
                    if (sensor.distance > -14 and sensor.distance < 14) {
                        switch (ground_mode) {
                            case Mode::Floor:     position.y += sensor.distance; break;
                            case Mode::RightWall: position.x += sensor.distance; break;
                            case Mode::Ceiling:   position.y -= sensor.distance; break;
                            case Mode::LeftWall:  position.x -= sensor.distance; break;
                        }

                        if (not sensor.flag) ground_angle = sensor.angle; else snap_angle();
                    }

                    // Check for slipping/falling when Ground Speed is too low on walls/ceilings.
                } break;
                case State::Rolling: { // "Rolling" means any time the Player is curled up into a ball on the ground.
                    // Adjust Ground Speed based on current Ground Angle (Rolling Slope Factors).
                    const fixed slope_factor = math::sign(ground_speed) == math::sign(math::sin(ground_angle))
                        ? SLOPE_FACTOR_ROLL_UP
                        : SLOPE_FACTOR_ROLL_DOWN;
                    ground_speed -= slope_factor * math::sin(ground_angle);

                    // Check for starting a jump.
                    if (input.key_pressed(Key::X)) {
                        speed.y -= JUMP_FORCE;
                    }

                    // Update Ground Speed based on directional input and apply friction.
                    // if (input.key_held(key_back))    ground_speed -= ACCELERATION_SPEED;
                    // if (input.key_held(key_forward)) ground_speed += ACCELERATION_SPEED;

                    // Push Sensor collision occurs.
                    // Which sensors are used varies based on the the sensor activation.
                    // This occurs before the Player's position physically moves, meaning he might not actually be touching
                    // the wall yet, the game accounts for this by adding the Player's X Speed and Y Speed to the sensor's position.
                    const auto push_mode = push_sensor_mode();

                    // Move the Player object
                    // Calculate X Speed and Y Speed from Ground Speed and Ground Angle.
                    // Update X Position and Y Position based on X Speed and Y Speed.

                    // Grounded Ground Sensor collision occurs.
                    // Updates the Player's Ground Angle.
                    // Align the Player to surface of terrain or become airborne if none found.
                    const auto ground_mode = ground_sensor_mode();

                    // Check for slipping/falling when Ground Speed is too low on walls/ceilings.
                } break;
                case State::Airborne: { // "Airborne" means when the Player is falling or jumping or otherwise not grounded.
                    // Check for jump button release (variable jump velocity).

                    // Check for turning Super.
                    if (false) {
                        // N/A as this is not a thing in Sonic 1 yet,
                        // there were only 6 chaos emeralds in the first game missing the final one required for the transformation.
                        // Preferably use the modern controls, so a unique button or mid-air second press of the jump button.
                        // Originally Sonic 2 made the brilliant decision to just automatically do this if you jump...
                    }

                    // Update X Speed based on directional input.
                    if (input.key_held(Key::Left))  speed.x -= AIR_ACCELERATION_SPEED;
                    if (input.key_held(Key::Right)) speed.x += AIR_ACCELERATION_SPEED;

                    // Apply air drag.
                    if (speed.y < fixed(0) and speed.y > fixed(-4)) {
                        speed.x -= math::trunc(speed.x / fixed(0, 125)) / fixed(256);
                    }

                    // Move the Player object
                    // Updates X Position and Y Position based on X Speed and Y Speed.

                    // Apply gravity.
                    // Update Y Speed by adding gravity to it.
                    // This happens after the Player's position was updated. This is an important detail for ensuring the Player's jump height is correct.
                    speed.y = std::max(fixed(16), speed.y + GRAVITY_FORCE);

                    // Check underwater for reduced gravity.
                    if (is_underwater()) {
                        // N/A as there is no water in the first stage.
                    }

                    // Rotate Ground Angle back to 0.
                    constexpr angle ANGLE_RETURN_SPEED = angle(3);

                    if (ground_angle > 180) {
                        ground_angle += ANGLE_RETURN_SPEED;
                    } else {
                        ground_angle -= ANGLE_RETURN_SPEED;
                    }

                    if (ground_angle > 356 and ground_angle < 4) {
                        ground_angle = angle(0);
                    }

                    // All air collision checks occur here.
                    // The sensors used depend on the sensor activation.
                    // Active Airborne Push Sensors check first, then the active Airborne Ground Sensors/Ceiling Sensors second.
                } break;
            }
        }

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

        void hud_draw(Ref<Image> target, Stage const& stage) const noexcept override {
            std::stringstream hud_string;
            hud_string
                << "SCORE  " << score << std::endl
                << "TIME  0:00" << std::endl
                << "RINGS  " << rings << std::endl;

            constexpr Color HUD_YELLOW = Color::rgba(255, 255, 10);

            std::string line;
            for (i32 y = 8; std::getline(hud_string, line); y += font::mine().height + 5) {
                target
                    | draw::draw(Text(line, font::sonic(), draw::color::BLACK), 8 + 1, y + 1)
                    | draw::draw(Text(line, font::sonic(), draw::color::BLACK), 8 + 1, y)
                    | draw::draw(Text(line, font::sonic(), HUD_YELLOW), 8, y);
            }
        }

        void debug_draw(std::stringstream& out, draw::Slice<Ref<Image>> target, Stage const& stage) const noexcept override {
            auto [ppx, ppy] = pixel_pos();

            auto aligned_target = target
                .shift(ppx, ppy);

            out << "Sonic:" << std::endl
                << "ground angle: " << (u16) ground_angle << std::endl
                << "speed: x: " << speed.x << " y: " << speed.y << std::endl;

            switch (state) {
                case State::Normal: {
                    const auto ground_mode = ground_sensor_mode();
                    stage.sense_draw(this, -width_radius(), height_radius(), Stage::SensorDirection::Down, ground_mode, aligned_target, draw::color::pico::LIME);
                    stage.sense_draw(this,  width_radius(), height_radius(), Stage::SensorDirection::Down, ground_mode, aligned_target, draw::color::pico::GREEN);
                } break;
                case State::Rolling: {

                } break;
                case State::Airborne: {

                } break;
            }
        }
    };
}
