#include "Game.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <atomic>

using namespace prog;

std::atomic<bool> IS_RUNNING = false;

void prog::run(Game& game, char const* title, i32 width, i32 height) {
    // FIXME: This is a hack due to SDL2 being broken on linux.
    {
        width *= 2;
        height *= 2;
    }

    if (IS_RUNNING.load()) {
        throw RunError { .reason = RunError::Reason::AlreadyRunning };
    } else {
        IS_RUNNING.store(true);
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
    Context ctx;

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
        game.update(0);
        game.draw(ctx);
        SDL_GL_SwapWindow(window);
    }
end:

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    IS_RUNNING.store(false);
}
