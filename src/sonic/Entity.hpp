// Created by Lua (TeamPuzel) on May 26th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <core>
#include "Fixed.hpp"

namespace sonic {
    enum class EntityId : u16 {

    };

    /// A dynamic game object.
    ///
    /// These assume 60hz updates for multiple reasons:
    /// - 8 bit fixed point arithmetic doesn't work with delta time.
    /// - The original games worked like this so it's more accurate.
    ///
    /// Updates happen in a set order:
    /// - The primary entity logic.
    /// - Non primary entity logic.
    /// - Universal update logic.
    class Entity {
      public:
        core::Vector<fixed, 2> position;

        /// Called on all entities each tick.
        virtual void update() {}
        /// Called on the primary entity each tick.
        virtual void update_primary() {}
        /// Called on all non primary entities each tick.
        virtual void update_computer() {}
    };

    class Sonic final : public Entity {};
}
