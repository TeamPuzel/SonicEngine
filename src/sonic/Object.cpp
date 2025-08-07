// Created by Lua (TeamPuzel) on May 26th 2025.
// Copyright (c) 2025 All rights reserved.
//
// Because C++ is a terrible language some object update implementations need to be moved here.
// Since they are virtual it's of no consequence to the compiler's ability to optimize but it is still
// very inconvenient. C++ has no redeeming qualities tbh.
#include "Object.hpp"
#include "Stage.hpp"

using namespace sonic;

void Sonic::update(rt::Input const& input, Stage& stage) {
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

    // TODO: More robust and composable solution, more importantly this must handle mirroring.
    const auto key_left  = Key::Left;
    const auto key_right = Key::Right;
    const auto key_up    = Key::Up;
    const auto key_down  = Key::Down;
    const auto key_jump  = Key::X;

    // This is a faithful implementation of classic 16 bit sonic physics based directly on the Sonic Physics Guide.
    // The comments come from the guide and provide a clear outline of what's going on.
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
            if (input.key_pressed(key_jump)) {
                speed.y -= JUMP_FORCE;
            }

            // Update Ground Speed based on directional input and apply friction/deceleration and the speed cap.
            if (const auto left = input.key_held(key_left), right = input.key_held(key_right); left or right) {
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
            } else {
                ground_speed -= std::min(math::abs(ground_speed), FRICTION_SPEED) * math::sign(ground_speed);
            }

            // Check for starting crouching, balancing on ledges, etc.

            // Push Sensor collision occurs.
            // Which sensors are used varies based on the the sensor activation.
            // This occurs before the Player's position physically moves, meaning they might not actually be touching
            // the wall yet, the game accounts for this by adding the Player's X Speed and Y Speed to the sensor's position.
            const auto push_mode = push_sensor_mode();

            // Check for starting a roll.
            if ((input.key_held(key_down)) and ground_speed >= fixed(0, 128)) {
                state = State::Rolling;
            }

            // Handle camera boundaries (keep the Player inside the view and kill them if they touch the kill plane).
            // NOTE: This will just handle the kill plane, my implementation inherently handles the camera unlike the original.

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

            // Check for slipping/falling when Ground Speed is too low on walls/ceilings.
        } break;
        case State::Rolling: { // "Rolling" means any time the Player is curled up into a ball on the ground.
            // Adjust Ground Speed based on current Ground Angle (Rolling Slope Factors).
            const fixed slope_factor = math::sign(ground_speed) == math::sign(math::sin(ground_angle))
                ? SLOPE_FACTOR_ROLL_UP
                : SLOPE_FACTOR_ROLL_DOWN;
            ground_speed -= slope_factor * math::sin(ground_angle);

            // Check for starting a jump.
            if (input.key_pressed(key_jump)) {
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

            // Handle camera boundaries (keep the Player inside the view and kill them if they touch the kill plane).

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
            if (input.key_held(key_left))  speed.x -= AIR_ACCELERATION_SPEED;
            if (input.key_held(key_right)) speed.x += AIR_ACCELERATION_SPEED;

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
