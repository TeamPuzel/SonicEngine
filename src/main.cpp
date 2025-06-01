// Created by Lua (TeamPuzel) on May 26th 2025.
// Copyright (c) 2025 All rights reserved.
#include <core>
#include <core::draw>

#include "sonic/runtime/Game.hpp"

class SonicCD final {
  public:
    void update() {}
    void draw(core::draw::SizedMutableDrawable auto& target) const {
        target | core::draw::clear();
    }
};

auto main() -> i32 {
    SonicCD game;
    sonic::rt::run(game, "Sonic");
}
