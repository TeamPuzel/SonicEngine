#include "Context.hpp"
#include <SDL_opengl.h>

using namespace prog;

void Context::clear() {
    glClear(GL_COLOR_BUFFER_BIT);
}
