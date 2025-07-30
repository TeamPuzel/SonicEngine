// Created by Lua (TeamPuzel) on May 26th 2025.
// Copyright (c) 2025 All rights reserved.
#include <primitive>
#include <draw>
#include <rt>
#include <sonic>

class SonicGame final {
    draw::Image tilemap;

  public:
    SonicGame() {
        // this->tilemap = draw::TgaImage::from(sonic::rt::load("tilemap.tga"))
        //     | draw::flatten<draw::Image>();
    }

    void update() {}

    void draw(draw::SizedMutableDrawable auto& target) const {
        target
            | draw::clear(draw::color::pico::DARK_BLUE)
            | draw::pixel(1, 1, draw::color::pico::RED);
    }
};

auto main() -> i32 {
    SonicGame game;
    rt::run(game, "Sonic", 4);
}
