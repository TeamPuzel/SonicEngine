// Created by Lua (TeamPuzel) on May 26th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <core>
#include "Scene.hpp"
#include "Entity.hpp"
#include "Tile.hpp"

namespace sonic {
    struct StageData final {};

    /// A coroutine class representing the game state of a loaded stage.
    class Stage final : Scene {
        core::Array<Entity*> entities;
        core::Array<Tile const*> tiles;

      public:
        void update() override {}
        void draw(core::draw::dyn::SizedMutableDrawable& target) const override {}

        auto load(StageData const& data) -> Stage {
            core::todo();
        };

        auto load(char const* data_path) -> Stage {
            core::todo();
        }
    };
}
