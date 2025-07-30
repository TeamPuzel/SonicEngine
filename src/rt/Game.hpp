// Created by Lua (TeamPuzel) on May 29th 2025.
// Copyright (c) 2025 All rights reserved.
//
// The game header defines the traits of a game and provides a default game executor.
#pragma once
#include <primitive>
#include <draw>
#include <concepts>
#include <string>
#include <optional>
#include <atomic>
#include <SDL3/SDL.h>

namespace rt {
    /// Identifies keyboard keys.
    ///
    /// Only baseline keys available on most small keyboards are exposed.
    enum class Key : u8 { A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z };

    /// Identifies mouse buttons.
    ///
    /// Only left and right are recognized as anything else is beyond baseline.
    enum class MouseButton : u8 { Left, Right };

    /// A mouse position relative to the context origin.
    struct MousePosition final {
        f32 x, y;
    };

    /// Defines a game runnable by a game executor. The default is `run(game)`.
    ///
    /// This does not use virtual dispatch because that would require the draw method
    /// to also be runtime polymorphic. This is opaque and prevents optimizing away the drawing
    /// for no benefit, as this type is never actually used in a runtime polymorphic way.
    /// It can simply be type erased and executed with the compiler having full insight.
    ///
    /// The catch is, without C++20 modules, this means the run implementation must be a template,
    /// so it is impossible for it to avoid exposing SDL includes, but that's an arbitrary
    /// issue caused by being forced to support old C++.
    ///
    /// TODO(last commit): Convert concept to type trait.
    template <typename Self, typename Target = draw::Image>
    concept Game =
        draw::SizedMutableDrawable<Target> and requires(Self& game_mut, Self const& game, Target& target) {
            { game_mut.update() } -> std::same_as<void>;
            { game.draw(target) } -> std::same_as<void>;
        };

    /// An error raised while running the game using the default executor.
    struct RunError final {
        /// The cause of the error.
        enum class Reason {
            AlreadyRunning,
            CouldNotInitializeSdl,
            CouldNotCreateWindow,
            CouldNotCreateContext [[deprecated]],
            CouldNotCreateRenderer,
            CouldNotCreateTexture,
            CouldNotRenderTexture,
            CouldNotPresentToWindow,
        } reason;
        /// The description obtained from SDL, only present for SDL errors.
        std::optional<std::string> description { std::nullopt };
    };

    /// Runs a game in the environment.
    ///
    /// This method was moved from Game into an environment message.
    /// A game cannot run itself, it is run by the platform it's on
    /// and can be run in many ways, this is just one implementation.
    static void run(Game auto& game, char const* title, i32 scale, i32 width, i32 height) {
        static std::atomic<bool> is_running = false;

        if (is_running.load()) {
            throw RunError { .reason = RunError::Reason::AlreadyRunning };
        } else {
            is_running.store(true);
        }

        if (not SDL_Init(SDL_INIT_VIDEO)) {
            throw RunError {
                .reason = RunError::Reason::CouldNotInitializeSdl,
                .description = SDL_GetError(),
            };
        }

        auto const window = SDL_CreateWindow(
            title, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
        );
        if (not window) {
            throw RunError {
                .reason = RunError::Reason::CouldNotCreateWindow,
                .description = SDL_GetError(),
            };
        }
        SDL_SetWindowMinimumSize(window, width, height);

        // A simple SDL provided renderer.
        //
        // We are already using SDL and there is no need to bother setting up OpenGL just to
        // render to a texture and blit it to the screen. There's really no need for hardware
        // acceleration to begin with, the only API that should be used is a platform specific
        // synchronization primitive such as CADisplayLink on macOS. That would also properly
        // support variable refresh rates on such platforms which I am unsure SDL actually handles.
        auto const renderer = SDL_CreateRenderer(window, nullptr);
        if (not renderer) {
            throw RunError {
                .reason = RunError::Reason::CouldNotCreateRenderer,
                .description = SDL_GetError(),
            };
        }
        // We can discard the error, it is inefficient not to use vsync but if a platform doesn't support it
        // we have much greater issues than rendering too fast and it's impressive we even got this far.
        (void) SDL_SetRenderVSync(renderer, 1);

        // Not const because we reallocate on window resize.
        //
        // Interesting note: stretching a pixel texture is the best way to get sharp pixels
        // with resizable windows on modern displays. You do very much notice blurry pixels,
        // you do not notice when the pixel ratio isn't quite square.
        //
        // Sure, on extremely low resolution displays this was once much more extreme, but no
        // operating system actually supports those anyhow. There is no reason to compromise here
        // as for whatever reason a lot of games seem to do.
        SDL_Texture* texture = nullptr;

        /// Reallocates the texture.
        auto const resize_texture = [&texture, renderer] (i32 w, i32 h) -> void {
            if (texture) {
                SDL_DestroyTexture(texture);
            }

            texture = SDL_CreateTexture(
                renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, w, h
            );
            if (not texture) {
                throw RunError {
                    .reason = RunError::Reason::CouldNotCreateTexture,
                    .description = SDL_GetError(),
                };
            }
            SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
        };
        resize_texture(width / scale, height / scale);

        SDL_Event event;
        auto target = draw::Image(width / scale, height / scale);

        while (true) {
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                    case SDL_EVENT_QUIT:
                        goto end;
                    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                        // We are explicitly using the scaled window size and not the
                        // GetWindowSizeInPixels(window:w:h:) call because we do actually want to scale
                        // our own pixel scale by the display scale.
                        i32 w, h;
                        SDL_GetWindowSize(window, &w, &h);
                        resize_texture(w / scale, h / scale);
                        target.resize(w / scale, h / scale);
                        break;
                    default:
                        break;
                }
            }

            // FIXME: Updates should be ensured as 60hz.
            game.update();
            game.draw(target);

            SDL_RenderClear(renderer);

            SDL_UpdateTexture(texture, nullptr, target.raw(), u16(target.width() * sizeof(draw::Color)));

            if (not SDL_RenderTexture(renderer, texture, nullptr, nullptr)) {
                throw RunError {
                    .reason = RunError::Reason::CouldNotRenderTexture,
                    .description = SDL_GetError(),
                };
            }

            if (not SDL_RenderPresent(renderer)) {
                throw RunError {
                    .reason = RunError::Reason::CouldNotPresentToWindow,
                    .description = SDL_GetError(),
                };
            }
        }
    end:

        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        is_running.store(false);
    }

    /// Runs a game in the environment.
    ///
    /// This method was moved from Game into an environment message.
    /// A game cannot run itself, it is run by the platform it's on
    /// and can be run in many ways, this is just one implementation.
    ///
    /// This overload uses the default window size of 800x600.
    inline void run(Game auto& game, char const* title = "Window", i32 scale = 4) {
        run(game, title, scale, 800, 600);
    }
}
