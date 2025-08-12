// Created by Lua (TeamPuzel) on August 11th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <sonic>

namespace sonic {
    /// Collectable rings.
    class Ring final : public Object, public Codable<Ring> {
      public:
        static constexpr i32 ANIMATION_STEP = 8;

        auto sprite(rt::Input const& input) const noexcept -> Sprite override {
            return Sprite { 12 + ((i32) input.counter() / ANIMATION_STEP % 4), 12, 16, 16 };
        }
    };
}
