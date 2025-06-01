#pragma once
#include <core>
#include "Context.hpp"
#include <string>
#include <optional>

namespace prog {
    class Context;

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

    /// Abstract game runnable by a game executor. The provided default is `run(game)`.
    class Game {
      public:
        Game() = default;
        Game(Game const&) = delete;
        Game(Game&&) = delete;
        Game& operator=(Game const&) = delete;
        Game& operator=(Game&&) = delete;
        virtual ~Game() {}

        virtual void update(float elapsed_sec) {}
        virtual void draw(Context& ctx) const {}

        virtual void on_key_down(Key key) {}
        virtual void on_key_up(Key key) {}
        virtual void on_mouse_move(MousePosition pos) {}
        virtual void on_mouse_down(MousePosition pos, MouseButton btn) {}
        virtual void on_mouse_up(MousePosition pos, MouseButton btn) {}
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
    void run(Game& game, char const* title, i32 width, i32 height);

    /// Runs a game in the environment.
    ///
    /// This method was moved from Game into an environment message.
    /// A game cannot run itself, it is run by the platform it's on
    /// and can be run in many ways, this is just one implementation.
    ///
    /// This overload uses the default window size of 800x600.
    inline void run(Game& game, char const* title) {
        run(game, title, 800, 600);
    }
}
