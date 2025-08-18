// Created by Lua (TeamPuzel) on August 11th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <iomanip>
#include <sonic>
#include <sstream>
#include "pickup/Ring.hpp"

namespace sonic {
    /// The player entity representing Sonic himself.
    class Sonic final : public Object, public DefaultCodable<Sonic> {
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

        /// Determines if sonic can adjust mid-air trajectory.
        bool air_control { true };
        /// Determines if we are in the air due to a jump.
        bool manual_jump { false };
        /// Determines if Sonic is rolled up after a jump.
        bool rolled_up { false };
        /// The state used while charging a spindash.
        fixed spin_rev { 0 };
        /// Used to lock controls for slipping.
        u8 control_lock { 0 };
        /// The invulnerability timer.
        u8 invulnerability { 0 };

        enum class DamageState {
            None,
            FlyingBack,
            Pending,
        } damage_state { DamageState::None };
        /// Associated member of DamageState::Pending if C++ had type-safe tagged unions.
        point<fixed> damaged_by_position;

        enum class Animation {
            Standing,
            Walking,
            Running,
            Rolling,
            Skidding,
            Hurt,
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

        [[clang::always_inline]] [[gnu::const]]
        auto width_radius() const noexcept -> i32 {
            switch (state) {
                case State::Normal:   return 9;
                case State::Rolling:  return 7;
                case State::Airborne: return 7;
            }
        }

        [[clang::always_inline]] [[gnu::const]]
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
        static constexpr fixed HURT_GRAVITY_FORCE      = fixed(0, 48 );

        auto hitbox() const noexcept -> Hitbox override {
            if (rolled_up) {
                return Hitbox::of_radii(7, 14);
            } else {
                return Hitbox::of_radii(9, 19);
            }
        }

        void collide_with(Object* other) noexcept override {
            if (const auto ring = flat_cast<Ring>(other)) {
                ring->pick_up();
                rings += 1;
            }

            if (const auto damaging = other->trait<DamagesPlayer>()) {
                if ((not rolled_up or damaging->bypass_protection()) and invulnerability == 0) {
                    damage_state = DamageState::Pending;
                    damaged_by_position = other->position;
                } else if (const auto takes_damage = other->trait<TakesDamageFromPlayer>()) {
                    takes_damage->damage();
                }
            }
        }

        void update(rt::Input const& input, Stage& stage) noexcept override {
            using rt::Key;
            using math::angle;

            // Count up the elapsed level seconds.
            if (input.counter() % 60 == 0) timer += 1;

            // Special case overriding any player update logic at all with debug flight.
            if (stage.movement_debug) {
                if (input.key_held(Key::Up))    position.y -= 10;
                if (input.key_held(Key::Down))  position.y += 10;
                if (input.key_held(Key::Left))  position.x -= 10;
                if (input.key_held(Key::Right)) position.x += 10;
                return;
            }

            if (damage_state == DamageState::Pending) {
                i32 ring_counter = 0;
                angle ring_starting_angle = 100;
                angle ring_angle = ring_starting_angle;
                bool ring_flip = false;
                fixed ring_speed = 4;

                // Perform loop while the ring counter is less than number of lost rings.
                while (ring_counter < std::min(32u, rings)) {
                    // Create a bouncing ring object at the Player's X and Y Position.
                    auto ring = Ring::scatter(position, {
                        +math::cos(ring_angle) * ring_speed,
                        -math::sin(ring_angle) * ring_speed,
                    });

                    // Every ring created will move at the same angle as the other in the current pair, but flipped to the other side of the circle.
                    // We increment the angle on every other ring which makes 2 even rings on either side.
                    if (ring_flip) {
                        ring->speed.x *= -1;
                        ring_angle += 22;
                    }

                    stage.add(std::move(ring));

                    // Toggle flip
                    ring_flip = !ring_flip;

                    // Increment counter
                    ring_counter += 1;

                    // If we are halfway start second circle of rings with lower speed.
                    if (ring_counter == 16) {
                        ring_speed = 2;
                        ring_angle = ring_starting_angle; // Reset the angle.
                    }
                }

                rings = 0;
                invulnerability = 120;

                state = State::Airborne;
                rolled_up = false;
                manual_jump = false;
                air_control = false;
                damage_state = DamageState::FlyingBack;

                fixed decide_direction = math::sign(position.x - damaged_by_position.x);
                if (decide_direction == 0) decide_direction = 1;

                speed.x =  HURT_X_FORCE * decide_direction;
                speed.y = -HURT_Y_FORCE;

                return;
            }

            if (invulnerability != 0 and damage_state != DamageState::FlyingBack) invulnerability -= 1;

            // This is a faithful implementation of classic 16 bit sonic physics based directly on the Sonic Physics Guide.
            // Many comments come from the guide and provide a clear outline of what's going on.
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
                    if (input.key_pressed(Key::X)) {
                        speed.x -= JUMP_FORCE * math::sin(ground_angle);
                        speed.y -= JUMP_FORCE * math::cos(ground_angle);

                        // The original game just kind of dropped the frame and returned early, only
                        // processing the jump with a frame delay. Well, I don't care to fix this at the moment.
                        state = State::Airborne;
                        rolled_up = true;
                        manual_jump = true;
                        return;
                    }

                    {
                        const auto left = input.key_held(Key::Left), right = input.key_held(Key::Right);

                        // Update Ground Speed based on directional input and apply friction/deceleration and the speed cap.
                        if ((left or right) and control_lock == 0) {
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
                    // ? These are not states just animations (well, crouching does change the hitbox in Sonic 1 but whatever)

                    // Push Sensor collision occurs.
                    // Which sensors are used varies based on the the sensor activation.
                    // This occurs before the Player's position physically moves, meaning they might not actually be touching
                    // the wall yet, the game accounts for this by adding the Player's X Speed and Y Speed to the sensor's position.
                    const auto push_mode = push_sensor_mode();

                    // Check for starting a roll.
                    if ((input.key_held(Key::Down)) and math::abs(ground_speed) >= fixed(0, 128)) {
                        state = State::Rolling;
                        rolled_up = true;
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
                    {
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
                        } else {
                            state = State::Airborne;
                            rolled_up = false;
                            manual_jump = false;
                        }
                    }

                    if (control_lock != 0) control_lock -= 1;

                    // Check for slipping/falling when Ground Speed is too low on walls/ceilings.
                    if (control_lock == 0 and ground_angle < 315 and ground_angle > 45 and math::abs(ground_speed) < fixed(2, 128)) {
                        state = State::Airborne;
                        rolled_up = false;
                        manual_jump = false;
                        // ground_speed = 0;
                        control_lock = 30;
                    }
                } break;
                case State::Rolling: { // "Rolling" means any time the Player is curled up into a ball on the ground.
                    // Adjust Ground Speed based on current Ground Angle (Rolling Slope Factors).
                    const fixed slope_factor = math::sign(ground_speed) == math::sign(math::sin(ground_angle))
                        ? SLOPE_FACTOR_ROLL_UP
                        : SLOPE_FACTOR_ROLL_DOWN;
                    ground_speed -= slope_factor * math::sin(ground_angle);

                    // Check for starting a jump.
                    // Most classic games lock controls when jumping while rolling but I prefer the Sonic CD controls.
                    if (input.key_pressed(Key::X)) {
                        speed.x -= JUMP_FORCE * math::sin(ground_angle);
                        speed.y -= JUMP_FORCE * math::cos(ground_angle);

                        // The original game just kind of dropped the frame and returned early, only
                        // processing the jump with a frame delay. Well, I don't care to fix this at the moment.
                        state = State::Airborne;
                        rolled_up = true;
                        air_control = false;
                        manual_jump = true;
                        return;
                    }

                    // Update Ground Speed based on directional input and apply friction.
                    {
                        const auto left = input.key_held(Key::Left), right = input.key_held(Key::Right);

                        // Update Ground Speed based on directional input and apply friction/deceleration and the speed cap.
                        // Note that this was weird in the original and the cap was applied to the x speed and not
                        // the ground speed which is unnecessary and weird so it's not done here.
                        if (left or right) {
                            if (left and not right and ground_speed > 0) {
                                ground_speed -= ROLL_DECELERATION_SPEED;
                            }

                            if (right and not left and ground_speed < 0) {
                                ground_speed += ROLL_DECELERATION_SPEED;
                            }
                        }

                        if (left == right or
                            left and not right and ground_speed < 0 or
                            right and not left and ground_speed > 0
                        ) {
                            ground_speed -= std::min(math::abs(ground_speed), ROLL_FRICTION_SPEED) * math::sign(ground_speed);
                        }
                    }

                    // Push Sensor collision occurs.
                    // Which sensors are used varies based on the the sensor activation.
                    // This occurs before the Player's position physically moves, meaning he might not actually be touching
                    // the wall yet, the game accounts for this by adding the Player's X Speed and Y Speed to the sensor's position.
                    const auto push_mode = push_sensor_mode();

                    // Move the Player object
                    // Calculate X Speed and Y Speed from Ground Speed and Ground Angle.
                    // Update X Position and Y Position based on X Speed and Y Speed.
                    speed.x = ground_speed *  math::cos(ground_angle);
                    speed.y = ground_speed * -math::sin(ground_angle);
                    position += speed;

                    // Grounded Ground Sensor collision occurs.
                    // Updates the Player's Ground Angle.
                    // Align the Player to surface of terrain or become airborne if none found.
                    {
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
                        } else {
                            state = State::Airborne;
                            rolled_up = true;
                            manual_jump = false;
                        }
                    }

                    if (math::abs(ground_speed) < fixed(0, 128)) {
                        state = State::Normal;
                        rolled_up = false;
                    }

                    // Check for slipping/falling when Ground Speed is too low on walls/ceilings.
                    if (ground_angle < 315 and ground_angle > 45 and math::abs(ground_speed) < fixed(2, 128)) {
                        state = State::Airborne;
                        rolled_up = false;
                        // ground_speed = 0;
                        control_lock = 30;
                    }
                } break;
                case State::Airborne: { // "Airborne" means when the Player is falling or jumping or otherwise not grounded.
                    // Check for jump button release (variable jump velocity).
                    // TODO: You could re-press the button, fix this.
                    if (not input.key_held(Key::X) and speed.y < -4 and manual_jump) {
                        speed.y = -4;
                    }

                    // Check for turning Super.
                    if (false) {
                        // N/A as this is not a thing in Sonic 1 yet,
                        // there were only 6 chaos emeralds in the first game missing the final one required for the transformation.
                        // Preferably use the modern controls, so a unique button or mid-air second press of the jump button.
                        // Originally Sonic 2 made the brilliant decision to just automatically do this when you jump...
                    }

                    // Update X Speed based on directional input.
                    if (air_control) {
                        const auto left = input.key_held(Key::Left), right = input.key_held(Key::Right);

                        if (left or right) {
                            if (left and not right) {
                                if (speed.x > 0) {
                                    speed.x -= AIR_ACCELERATION_SPEED;
                                } else if (speed.x > -TOP_SPEED) {
                                    speed.x -= AIR_ACCELERATION_SPEED;
                                    if (speed.x <= -TOP_SPEED) speed.x = -TOP_SPEED;
                                }
                            }

                            if (right and not left) {
                                if (speed.x < 0) {
                                    speed.x += AIR_ACCELERATION_SPEED;
                                } else if (speed.x < TOP_SPEED) {
                                    speed.x += AIR_ACCELERATION_SPEED;
                                    if (speed.x >= TOP_SPEED) speed.x = TOP_SPEED;
                                }
                            }
                        }
                    }

                    // Apply air drag.
                    if (speed.y < fixed(0) and speed.y > fixed(-4)) {
                        speed.x -= math::trunc(speed.x / fixed(0, 125)) / fixed(256);
                    }

                    // Move the Player object
                    // Updates X Position and Y Position based on X Speed and Y Speed.
                    position += speed;

                    // Apply gravity.
                    // Update Y Speed by adding gravity to it.
                    // This happens after the Player's position was updated. This is an important detail for ensuring the Player's jump height is correct.
                    speed.y = std::min(fixed(16), speed.y + (damage_state == DamageState::FlyingBack ? HURT_GRAVITY_FORCE : GRAVITY_FORCE));

                    // Check underwater for reduced gravity.
                    if (is_underwater()) {
                        // N/A as there is no water in the first stage.
                    }

                    // Rotate Ground Angle back to 0.
                    static constexpr angle ANGLE_RETURN_SPEED = angle(3);

                    if (ground_angle > 180) {
                        ground_angle += ANGLE_RETURN_SPEED;
                    } else {
                        ground_angle -= ANGLE_RETURN_SPEED;
                    }

                    if (ground_angle > 340 or ground_angle < 20) {
                        ground_angle = angle(0);
                    }

                    // All air collision checks occur here.
                    // The sensors used depend on the sensor activation.
                    // Active Airborne Push Sensors check first, then the active Airborne Ground Sensors/Ceiling Sensors second.
                    // Air collision is always in floor mode.
                    {
                        const auto sensor_a = stage.sense(this, -width_radius(), height_radius(), Stage::SensorDirection::Down);
                        const auto sensor_b = stage.sense(this,  width_radius(), height_radius(), Stage::SensorDirection::Down);

                        const auto sensor = sensor_b.distance < sensor_a.distance
                            ? sensor_b
                            : sensor_a;

                        if (sensor.distance > -14 and sensor.distance < 14 and speed.y > 0) {
                            state = State::Normal;
                            rolled_up = false;
                            air_control = true;
                            position.y += sensor.distance;
                            if (not sensor.flag) ground_angle = sensor.angle; else snap_angle();

                            // Something resembling the original calculation is below.
                            // I have no clue why they did that, must be some esoteric m68k specific explanation?
                            // (Now that I think about it it was probably to avoid the square root?)

                            // FIXME:
                            // The documentation says ambiguous nonsense like "if mostly right or left".
                            // I have no clue what that means so let's just compare with this for now. Ugh.
                            const fixed no_clue_what_this_is = speed.y; // fixed(1, 128);

                            if (ground_angle > 340 or ground_angle < 20) {
                                ground_speed = speed.x;
                            } else if (ground_angle > 315 or ground_angle < 45) {
                                ground_speed = math::abs(speed.x) > no_clue_what_this_is
                                    ? speed.x
                                    : speed.y * fixed(0, 128) * -math::sign(math::sin(ground_angle));
                            } else {
                                ground_speed = math::abs(speed.x) > no_clue_what_this_is
                                    ? speed.x
                                    : speed.y * -math::sign(math::sin(ground_angle));
                            }

                            damage_state = DamageState::None;
                        }
                    }
                } break;
            }
        }

        auto sprite(rt::Input const& input) const noexcept -> Sprite override {
            using rt::Key;

            if (damage_state != DamageState::FlyingBack) {
                if (state == State::Airborne and not rolled_up) {
                    if (input.key_held(Key::Left))  mirror_x = true;
                    if (input.key_held(Key::Right)) mirror_x = false;
                } else if (state != State::Rolling) {
                    if (ground_speed < 0 and input.key_held(Key::Left))  mirror_x = true;
                    if (ground_speed > 0 and input.key_held(Key::Right)) mirror_x = false;
                }
            }

            if (damage_state == DamageState::FlyingBack) {
                if (animator.play(Animation::Hurt, 2, 8)) {
                    anim_x = 4, anim_y = 10;
                }

                animator.update();
            } else if (not rolled_up) {
                const fixed abs_speed = math::abs(ground_speed);

                if ((abs_speed > 4 or animator.is(Animation::Skidding)) and state != State::Airborne and (
                    ground_speed < 0 and input.key_held(Key::Right) and not input.key_held(Key::Left) or
                    ground_speed > 0 and input.key_held(Key::Left) and not input.key_held(Key::Right)
                )) {
                    if (animator.play(Animation::Skidding, 2, 8)) {
                        anim_x = 6, anim_y = 7;
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
                    animator.set_speed((i32) math::floor(std::max(fixed(0), 8 - abs_speed)));

                    // if (is_half_steep()) {
                    //     anim_y = animator.is(Animation::Walking) ? 8 : 10;
                    // } else {
                    //     anim_y = animator.is(Animation::Walking) ? 7 : 9;
                    // }
                }

                animator.update();
            } else {
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
            }

            return Sprite {
                anim_x + (i32) animator.at(), anim_y,
                64, 64,
                mirror_x,
                false,
                (u8) ground_sensor_mode(),
            };
        }

        auto camera_buffer() const noexcept -> CameraBuffer override {
            switch (state) {
                case State::Normal:
                case State::Rolling:  return { 8, 0, math::abs(ground_speed) >= 8 ? 16 : 6 };
                case State::Airborne: return { 8, 32, 16 };
            }
        }

        void hud_draw(Io& io, Ref<Image> target, Stage const& stage) const noexcept override {
            std::stringstream hud_string;
            hud_string
                << "SCORE  " << std::setw(7) << score << std::endl
                << "TIME  " << timer / 60 << ":" << std::setfill('0') << std::setw(2) << timer % 60 << std::endl
                << "RINGS  " << std::setfill(' ') << std::setw(3) << rings << std::endl;

            constexpr Color HUD_YELLOW = Color::rgba(255, 255, 10);

            std::string line;
            for (i32 y = 8; std::getline(hud_string, line); y += font::mine(io).height + 5) {
                target
                    | draw::draw(Text(line, font::sonic(io), draw::color::BLACK), 8 + 1, y + 1)
                    | draw::draw(Text(line, font::sonic(io), draw::color::BLACK), 8 + 1, y)
                    | draw::draw(Text(line, font::sonic(io), HUD_YELLOW), 8, y);
            }
        }

        void debug_draw(Io& io, std::stringstream& out, draw::Slice<Ref<Image>> target, Stage const& stage) const noexcept override {
            auto [ppx, ppy] = pixel_pos();

            auto aligned_target = target
                .shift(ppx, ppy);

            out << "Sonic:" << std::endl
                << "ground angle: " << (u16) ground_angle << std::endl
                << "speed: x: " << speed.x << " y: " << speed.y << std::endl
                << "ground speed: " << ground_speed << std::endl;

            switch (state) {
                case State::Normal:   out << "state: Normal"   << std::endl; break;
                case State::Rolling:  out << "state: Rolling"  << std::endl; break;
                case State::Airborne: out << "state: Airborne" << std::endl; break;
            }

            // As usual C++ is taking what was perfectly sound in C (formatting) and making stupid, error prone assumptions.
            // I wish I at least had std::format but this is decade old C++17, ugh.
            out << "control lock: " << (u16) control_lock << std::endl;

            target
                | draw::line(ppx, ppy, ppx + (i32) speed.x * 3, ppy + (i32) speed.y * 3)
                | draw::pixel(ppx + (i32) speed.x * 3, ppy + (i32) speed.y * 3, draw::color::pico::RED);

            switch (state) {
                case State::Normal: {
                    const auto ground_mode = ground_sensor_mode();
                    stage.sense_draw(this, -width_radius(), height_radius(), Stage::SensorDirection::Down, ground_mode, aligned_target, draw::color::pico::LIME);
                    stage.sense_draw(this,  width_radius(), height_radius(), Stage::SensorDirection::Down, ground_mode, aligned_target, draw::color::pico::GREEN);
                } break;
                case State::Rolling: {

                } break;
                case State::Airborne: {
                    const auto ground_mode = ground_sensor_mode();
                    stage.sense_draw(this, -width_radius(), height_radius(), Stage::SensorDirection::Down, aligned_target, draw::color::pico::LIME);
                    stage.sense_draw(this,  width_radius(), height_radius(), Stage::SensorDirection::Down, aligned_target, draw::color::pico::GREEN);
                } break;
            }
        }
    };
}
