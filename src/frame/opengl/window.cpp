#include "frame/opengl/window.h"

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
#include "frame/opengl/none_opengl_window.h"
#include "frame/opengl/sdl_opengl_window.h"

namespace frame::opengl {

std::unique_ptr<WindowInterface> CreateSDL2OpenGL(std::pair<std::uint32_t, std::uint32_t> size) {
    auto window  = std::make_unique<SDLOpenGLWindow>(size);
    auto context = window->GetGraphicContext();
    if (!context) return nullptr;
    window->SetUniqueDevice(std::make_unique<Device>(context, size));
    return window;
}

std::unique_ptr<WindowInterface> CreateNoneOpenGL(std::pair<std::uint32_t, std::uint32_t> size) {
    auto window = std::make_unique<NoneOpenGLWindow>(size);
    auto context = window->GetGraphicContext();
    if (!context) return nullptr;
    window->SetUniqueDevice(std::make_unique<Device>(context, size));
    return window;
}

}  // End namespace frame::opengl.
