// Created by Lua (TeamPuzel) on May 29th 2025.
// Copyright (c) 2025 All rights reserved.
// 
// The game header defines the traits of a game and provides a default game executor.
#pragma once
#include <core>
#include <core::draw>

#include <concepts>
#include <string>
#include <optional>
#include <atomic>

#include <SDL.h>
#include <SDL_opengl.h>

namespace sonic::rt {
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
    /// so it is impossible for it to avoid exposing SDL/OpenGL includes, but that's an arbitrary
    /// issue caused by being forced to use old C++.
    ///
    /// TODO(last commit): Convert concept to type trait.
    template <typename Self, typename Target>
    concept Game =
        core::draw::SizedMutableDrawable<Target> and requires(Self& game_mut, Self const& game, Target& target) {
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
            CouldNotCreateContext,
        } reason;
        /// The description obtained from SDL, only present for SDL errors.
        std::optional<std::string> description { std::nullopt };
    };

    /// Runs a game in the environment.
    ///
    /// This method was moved from Game into an environment message.
    /// A game cannot run itself, it is run by the platform it's on
    /// and can be run in many ways, this is just one implementation.
    static void run(Game<core::draw::Image> auto& game, char const* title, i32 width, i32 height) {
        static std::atomic<bool> is_running = false;

        // FIXME: This is a hack due to SDL2 being broken on linux.
        {
            width *= 2;
            height *= 2;
        }

        if (is_running.load()) {
            throw RunError { .reason = RunError::Reason::AlreadyRunning };
        } else {
            is_running.store(true);
        }

        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            throw RunError {
                .reason = RunError::Reason::CouldNotInitializeSdl,
                .description = SDL_GetError(),
            };
        }

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

        auto const window = SDL_CreateWindow(
            title,
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            width,
            height,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL
        );
        if (not window) {
            throw RunError {
                .reason = RunError::Reason::CouldNotCreateWindow,
                .description = SDL_GetError(),
            };
        }
        SDL_SetWindowMinimumSize(window, width, height);

        auto const context = SDL_GL_CreateContext(window);
        if (not context) {
            throw RunError {
                .reason = RunError::Reason::CouldNotCreateContext,
                .description = SDL_GetError(),
            };
        }

        {
            SDL_GL_SetSwapInterval(1);

            glViewport(0, 0, width, height);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);
        }

        SDL_Event event;
        core::draw::Image target;

        while (true) {
            while (SDL_PollEvent(&event) != 0) {
                switch (event.type) {
                    case SDL_QUIT:
                        goto end;
                    case SDL_WINDOWEVENT:
                        switch (event.window.event) {
                            case SDL_WINDOWEVENT_SIZE_CHANGED:
                                i32 w, h;
                                SDL_GetWindowSizeInPixels(window, &w, &h);
                                glViewport(0, 0, w, h);
                                break;
                            default:
                                break;
                        }
                    default:
                        break;
                }
            }
            game.update();
            game.draw(target);
            SDL_GL_SwapWindow(window);
        }
    end:

        SDL_GL_DeleteContext(context);
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
    inline void run(Game<core::draw::Image> auto& game, char const* title) {
        run(game, title, 800, 600);
    }
}
