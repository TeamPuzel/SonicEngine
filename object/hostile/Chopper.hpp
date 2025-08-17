// Created by Lua (TeamPuzel) on August 12th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <sonic>

namespace sonic {
    class Chopper final : public Object, public DefaultCodable<Chopper> {
      public:
        Chopper() {
            add<DamagesPlayer>(DamagesPlayer::UnprotectedOnly);
            add<TakesDamageFromPlayer>(&Chopper::damage);
        }

        /// We need this in order to remember where we are bouncing from.
        point<fixed> initial_position;

        static constexpr fixed GRAVITY = fixed(0, 24);
        static constexpr fixed SPEED = fixed(7);

        static constexpr i32 ANIMATION_STEP = 8;

        auto hitbox() const noexcept -> Hitbox override {
            return Hitbox::of_radii(12, 16);
        }

        void damage() {

        }

        void update(rt::Input const& input, Stage& stage) noexcept override {
            if (position.y >= initial_position.y) {
                position = initial_position;
                speed.y = -SPEED;
            } else {
                speed.y += GRAVITY;
            }

            position += speed;
        }

        auto sprite(rt::Input const& input) const noexcept -> Sprite override {
            return Sprite { 0 + ((i32) input.counter() / ANIMATION_STEP % 2), 31, 32, 32 };
        };

        // We shall shadow the exported implementations to add our custom state.

        static auto rebuild(Chopper const& existing) -> Box<Object> {
            auto ret = DefaultCodable::rebuild(existing).cast<Chopper>();
            ret->initial_position = existing.initial_position;
            return ret;
        }

        static auto deserialize(rt::BinaryReader& reader, i32 x, i32 y) -> Box<Object> {
            auto ret = DefaultCodable::deserialize(reader, x, y).cast<Chopper>();
            ret->initial_position = ret->position;
            return ret;
        }
    };
}
