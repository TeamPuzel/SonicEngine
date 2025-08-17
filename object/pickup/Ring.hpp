// Created by Lua (TeamPuzel) on August 11th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <sonic>

namespace sonic {
    /// Collectable rings.
    class Ring final : public Object, public DefaultCodable<Ring> {
        bool is_collected { false };
        bool is_scattered { false };
        u8 collected_counter { 0 };
        u8 scattered_counter { 0 };
        bool did_bounce { false };

      public:
        static constexpr i32 STATIC_ANIMATION_STEP = 8;
        static constexpr i32 FAST_ANIMATION_STEP = 2;
        static constexpr fixed GRAVITY = fixed(0, 24);
        static constexpr fixed BOUNCE_COEFFICIENT = -fixed(0, 192);
        static constexpr i32 WIDTH_RADIUS = 8;
        static constexpr i32 HEIGHT_RADIUS = 8;

        /// Scattered rings need to be updated far off-screen or the player can catch up to long dropped rings
        /// breaking the illusion. Obviously, we absolutely should not do this to static rings, there's a ton
        /// of static rings in a level.
        auto force_active() const -> bool override {
            return is_scattered;
        }

        auto animation_step() const -> i32 {
            // The original works a little differently, computing the speed based on time elapsed.
            // The documentation is unclear to me but I think making the speed fast until the ring bounces works great.
            if (is_scattered and not is_collected and not did_bounce) {
                return FAST_ANIMATION_STEP;
            } else {
                return STATIC_ANIMATION_STEP;
            }
        }

        static auto scatter(point<fixed> position, point<fixed> speed) -> Box<Ring> {
            auto ret = Box<Ring>::make();
            ret->position = position;
            ret->is_scattered = true;
            ret->speed = speed;
            ret->assume_classname("Ring");
            return ret;
        }

        void pick_up() {
            is_collected = true;
        }

        auto hitbox() const noexcept -> Hitbox override {
            if (not is_collected) {
                // We can't collide with rings for 64 frames after they are scattered.
                if (not is_scattered or scattered_counter > 64) {
                    return Hitbox::of_radii(6, 6);
                } else {
                    return {};
                }
            } else {
                return {};
            }
        }

        void update(rt::Input const& input, Stage& stage) noexcept override {
            if (is_collected) {
                if (collected_counter == 4 * animation_step()) stage.remove(this);
                collected_counter += 1;
            }

            if (is_scattered) {
                speed.y += GRAVITY;

                // This check is intentionally botched :)
                //
                // The original game did it like this as an optimization but changing it would mean
                // changing the balance and collecting rings is already really easy.
                //
                // It doesn't actually feel that weird to have them fly off stage though, I like that a lot.
                if (
                    input.counter() % 4 == 0 and speed.y > 0 and
                    stage.sense(this, 0, HEIGHT_RADIUS, Stage::SensorDirection::Down).hit(14, 1)
                ) {
                    speed.y *= BOUNCE_COEFFICIENT;
                    did_bounce = true;
                }

                position += speed;

                if (not is_collected and scattered_counter == 255) {
                    stage.remove(this);
                }

                scattered_counter += 1;
            }
        }

        auto sprite(rt::Input const& input) const noexcept -> Sprite override {
            // Scrolls through 2 horizontally adjacent 4 frame animations depending on the collection state.
            return Sprite { (is_collected ? 4 : 0) + 12 + ((i32) input.counter() / animation_step() % 4), 12, 16, 16 };
        }

        void debug_draw(Io& io, std::stringstream& out, draw::Slice<Ref<Image>> target, Stage const& stage) const noexcept override {
            if (is_scattered) {
                auto [ppx, ppy] = pixel_pos();

                auto aligned_target = target
                    .shift(ppx, ppy);

                target
                    | draw::line(ppx, ppy, ppx + (i32) speed.x * 3, ppy + (i32) speed.y * 3)
                    | draw::pixel(ppx + (i32) speed.x * 3, ppy + (i32) speed.y * 3, draw::color::pico::RED);

                stage.sense_draw(this, 0, HEIGHT_RADIUS, Stage::SensorDirection::Down, aligned_target, draw::color::pico::LIME);
            }
        }

        static auto rebuild(Ring const& existing) -> Box<Object> {
            auto ret = DefaultCodable::rebuild(existing).cast<Ring>();
            ret->is_collected = existing.is_collected;
            ret->is_scattered = existing.is_scattered;
            ret->collected_counter = existing.collected_counter;
            ret->scattered_counter = existing.scattered_counter;
            return ret;
        }
    };
}
