// Created by Lua (TeamPuzel) on May 26th 2025.
// Copyright (c) 2025 All rights reserved.
#include <primitive>
#include <draw>
#include <rt>
#include <sonic>

class SonicGame final {
    draw::Image sheet;
    draw::Image height_arrays;
    draw::Image angle_sheet;
    draw::Image background;
    box<sonic::Scene> scene;

  public:
    SonicGame() {
        this->sheet = draw::TgaImage::from(rt::load("res/tilemap.tga")) | draw::flatten<draw::Image>();
        this->height_arrays = draw::TgaImage::from(rt::load("res/collision.tga")) | draw::flatten<draw::Image>();
        this->angle_sheet = draw::TgaImage::from(rt::load("res/angles.tga")) | draw::flatten<draw::Image>();
        this->background = draw::TgaImage::from(rt::load("res/background.tga")) | draw::flatten<draw::Image>();

        this->scene = sonic::Stage::load("res/1-1.stage", sonic::registry, std::as_const(height_arrays) | draw::as_ref());
    }

    void update(rt::Input const& input) {
        scene->update(input);
    }

    template <typename T> void draw(rt::Input const& input, T& target) const {
        static_assert(draw::SizedDrawable<T>::value and draw::MutableDrawable<T>::value);
        scene->draw(input, target, sheet, background);
    }
};

auto main() -> i32 {
    SonicGame game;
    rt::run(game, "Sonic", 4);
}
