// Created by Lua (TeamPuzel) on May 26th 2025.
// Copyright (c) 2025 All rights reserved.
#include <primitive>
#include <draw>
#include <rt>
#include <sonic>

using draw::Image;
using draw::TgaImage;

class SonicGame final {
    Image sheet;
    Image height_arrays;
    Image angle_sheet;
    Image background;
    Box<sonic::Scene> scene;

  public:
    SonicGame() {
        this->sheet = TgaImage::from(rt::load("res/tilemap.tga")) | draw::flatten<Image>();
        this->height_arrays = TgaImage::from(rt::load("res/collision.tga")) | draw::flatten<Image>();
        this->angle_sheet = TgaImage::from(rt::load("res/angles.tga")) | draw::flatten<Image>();
        this->background = TgaImage::from(rt::load("res/background.tga")) | draw::flatten<Image>();

        this->scene = sonic::Stage::load("res/1-1.stage", sonic::registry, height_arrays);
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
