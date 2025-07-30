// Created by Lua (TeamPuzel) on May 26th 2025.
// Copyright (c) 2025 All rights reserved.
//
// A simple abstraction for switching scenes at runtime.
#pragma once
#include <primitive>
#include <draw>

namespace sonic {
    /// A scene coroutine which can be run.
    ///
    /// Rendering is performed into an image rather than a dynamic target since this is
    /// significantly faster for obvous reasons and we control the runtime and can ensure it is one.
    struct Scene {
        /// Advances the state by 1/60 of a second.
        virtual void update() = 0;
        /// Called after update to mutate the render target.
        virtual void draw(draw::Image& target) const = 0;
    };
}
