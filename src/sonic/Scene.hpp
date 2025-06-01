// Created by Lua (TeamPuzel) on May 26th 2025.
// Copyright (c) 2025 All rights reserved.
// 
// A simple abstraction for switching scenes at runtime.
#pragma once
#include <core>
#include <core::draw>

namespace sonic {
    /// A scene coroutine which can be run.
    ///
    /// Rendering is performed into a dynamically dispatched drawable target.
    struct Scene {
        /// Advances the state by 1/60 of a second.
        virtual void update() = 0;
        /// Called after update to mutate the render target.
        virtual void draw(core::draw::dyn::SizedMutableDrawable& target) const = 0;
    };
}
