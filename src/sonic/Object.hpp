// Created by Lua (TeamPuzel) on May 26th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <primitive>
#include <math>

namespace sonic {
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
    /// The data of an entity is a match of the state described in the Sonic Physics Guide.
    class Object {
      public:
        math::point<fixed> position;
        math::point<fixed> velocity;
        u16 width_radius;
        u16 height_radius;
        fixed ground_velocity;
        math::angle ground_angle;

        /// Called on all entities each tick.
        virtual void update() {}
    };

    class Sonic final : public Object {};

    class Ring final : public Object {};

    class Spike final : public Object {};

    class Platform final : public Object {};

    class CollapsingCliff final : public Object {};

    class Monitor final : public Object {};

    class Goal final : public Object {};

    class Checkpoint final : public Object {};

    class Spring final : public Object {};

    class Chopper final : public Object {};

    class MotoBug final : public Object {};

    class Crabmeat final : public Object {};

    class BuzzBomber final : public Object {};

    class BlueNewtron final : public Object {};

    class GreenNewtron final : public Object {};

    class Animal final : public Object {};
}
