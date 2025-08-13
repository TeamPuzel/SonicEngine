// Created by Lua (TeamPuzel) on May 26th 2025.
// Copyright (c) 2025 All rights reserved.
#include <primitive>
#include <draw>
#include <rt>
#include <sonic>

using draw::Image;
using draw::TgaImage;

static std::atomic<bool> RELOAD_REQUESTED = false;

#if defined(__APPLE__) || defined(__linux__)
#include <signal.h>
void reload_handler(i32 signal) {
    if (signal == SIGUSR1) RELOAD_REQUESTED = true;
}
#endif

class SonicGame final {
    Image sheet;
    Image height_arrays;
    Image angle_sheet;
    Image background;
    Box<sonic::Scene> scene;

  public:
    SonicGame() {
        this->sheet = TgaImage::from(rt::io::load("res/tilemap.tga")) | draw::flatten<Image>();
        this->height_arrays = TgaImage::from(rt::io::load("res/collision.tga")) | draw::flatten<Image>();
        this->angle_sheet = TgaImage::from(rt::io::load("res/angles.tga")) | draw::flatten<Image>();
        this->background = TgaImage::from(rt::io::load("res/background.tga")) | draw::flatten<Image>();

        this->scene = sonic::Stage::load("res/1-1.stage", height_arrays);
    }

    void update(rt::Input const& input) {
        if (RELOAD_REQUESTED) {
            scene->hot_reload();
            RELOAD_REQUESTED = false;
        }
        scene->update(input);
    }

    void draw(rt::Input const& input, draw::Image& target) const {
        scene->draw(input, target, sheet, background);
    }
};

auto main() -> i32 {
    #if defined(__APPLE__) || defined(__linux__)
    struct sigaction sa;
    sa.sa_handler = reload_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // restart syscalls after signal
    sigaction(SIGUSR1, &sa, NULL);
    #endif

    SonicGame game;
    rt::run(game, "Sonic", 4);
}
