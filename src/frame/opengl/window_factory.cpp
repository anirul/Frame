#include "frame/opengl/window_factory.h"

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <fmt/core.h>

#include <chrono>
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "frame/opengl/device.h"
#include "frame/opengl/sdl_opengl_none.h"
#include "frame/opengl/sdl_opengl_window.h"

namespace frame::opengl {

std::unique_ptr<WindowInterface> CreateSDL2OpenGLWindow(glm::uvec2 size) {
    auto window  = std::make_unique<SDLOpenGLWindow>(size);
    auto context = window->GetGraphicContext();
    if (!context) return nullptr;
    window->SetUniqueDevice(std::make_unique<Device>(context, size));
    return window;
}

std::unique_ptr<WindowInterface> CreateSDL2OpenGLNone(glm::uvec2 size) {
    auto window = std::make_unique<SDLOpenGLNone>(size);
    auto context = window->GetGraphicContext();
    if (!context) return nullptr;
    window->SetUniqueDevice(std::make_unique<Device>(context, size));
    return window;
}

}  // End namespace frame::opengl.
