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
    SonicGame() {}

    void init(Io& io) {
        sheet         = TgaImage::from(io.read_file("res/tilemap.tga")) | draw::flatten<Image>();
        height_arrays = TgaImage::from(io.read_file("res/collision.tga")) | draw::flatten<Image>();
        angle_sheet   = TgaImage::from(io.read_file("res/angles.tga")) | draw::flatten<Image>();
        background    = TgaImage::from(io.read_file("res/background.tga")) | draw::flatten<Image>();
        scene = sonic::Stage::load(io, "res/1-1.stage", height_arrays);
    }

    void update(Io& io, rt::Input const& input) {
        if (RELOAD_REQUESTED) {
            scene->hot_reload(io);
            RELOAD_REQUESTED = false;
        }
        scene->update(io, input);
    }

    void draw(Io& io, rt::Input const& input, draw::Ref<draw::Image> target) const {
        scene->draw(io, input, target, sheet, background);
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
    rt::run(game, "Sonic", 3, 960, 672); // Let's default to a scale and window size matching the original game resolution.
}
