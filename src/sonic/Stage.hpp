// Created by Lua (TeamPuzel) on May 26th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <primitive>
#include <rt>
#include <vector>
#include "Scene.hpp"
#include "Object.hpp"

namespace sonic {
    struct StageData final {};

    struct Tile final {
        i32 x, y;
        bool mirror_x;
        bool mirror_y;
    };

    /// A coroutine class representing the state of a loaded stage.
    class Stage final : Scene {
        u32 width { 0 };
        u32 height { 0 };
        std::vector<Tile> foreground;
        std::vector<Tile> collision;
        std::vector<Object*> objects;

        Stage() {}

      public:
        void update() override {}
        void draw(draw::Image& target) const override {}

        auto load(char const* filename) -> Stage {
            auto ret = Stage();

            const auto data = rt::load(filename);
            auto reader = rt::BinaryReader::of(data);

            ret.width = reader.read<u32>();
            ret.height = reader.read<u32>();

            ret.foreground.reserve(ret.width * ret.height);
            ret.collision.reserve(ret.width * ret.height);

            for (auto tile : reader.read<Tile>(ret.width * ret.height)) {
                ret.foreground.push_back(tile);
            }

            for (auto tile : reader.read<Tile>(ret.width * ret.height)) {
                ret.collision.push_back(tile);
            }

            // TODO: Objects

            return ret;
        }
    };
}
