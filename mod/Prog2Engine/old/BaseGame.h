#pragma once
#include "structs.h"
#include <SDL2/SDL.h>
// https://BaseGameprogrammingpatterns.com/subclass-sandbox.html

namespace prog {

    class BaseGame {
      public:
        explicit BaseGame(Window const& window);
        BaseGame(BaseGame const& other) = delete;
        BaseGame& operator=(BaseGame const& other) = delete;
        BaseGame(BaseGame&& other) = delete;
        BaseGame& operator=(BaseGame&& other) = delete;
        virtual ~BaseGame();
        
        void Run();

        virtual void Update(float elapsedSec) {}
        virtual void Draw() const {}

        // Event handling
        virtual void ProcessKeyDownEvent(SDL_KeyboardEvent const& e) {}
        virtual void ProcessKeyUpEvent(SDL_KeyboardEvent const& e) {}
        virtual void ProcessMouseMotionEvent(SDL_MouseMotionEvent const& e) {}
        virtual void ProcessMouseDownEvent(SDL_MouseButtonEvent const& e) {}
        virtual void ProcessMouseUpEvent(SDL_MouseButtonEvent const& e) {}

        Rectf const& GetViewPort() const {
            return m_Viewport;
        }

      private:
        // DATA MEMBERS
        // The window properties
        Window const m_Window;
        Rectf const m_Viewport;
        // The window we render to
        SDL_Window* m_pWindow;
        // OpenGL context
        SDL_GLContext m_pContext;
        // Init info
        bool m_Initialized;
        // Prevent timing jumps when debugging
        float const m_MaxElapsedSeconds;

        // FUNCTIONS
        void InitializeGameEngine();
        void CleanupGameEngine();
    };

}
