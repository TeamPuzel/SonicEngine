// Created by Lua (TeamPuzel) on August 12th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <sonic>

namespace sonic {
    class MotoBug final : public Object, public DefaultCodable<MotoBug> {
      public:
        static constexpr fixed SPEED = fixed(1);
        static constexpr fixed WIDTH_RADIUS = fixed(8);
        static constexpr fixed HEIGHT_RADIUS = fixed(14);

        /// Motobugs move sideways and when they hit a wall they stop for a second and turn around.
        void update(rt::Input const& input, Stage& stage) noexcept override {
            // stage.sense(this, 0, HEIGHT_RADIUS, Stage::SensorDirection::Down);
        }

        auto sprite(rt::Input const&) const noexcept -> Sprite override {
            return Sprite {};
        };
    };
}
