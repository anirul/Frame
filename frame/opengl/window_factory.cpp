#include "frame/opengl/window_factory.h"

#include <SDL3/SDL.h>
#include <chrono>
#include <exception>
#include <glad/glad.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "frame/opengl/device.h"
#include "frame/opengl/sdl_opengl_none.h"
#include "frame/opengl/sdl_opengl_window.h"

namespace frame::opengl
{

std::unique_ptr<WindowInterface> CreateSDLOpenGLWindow(glm::uvec2 size)
{
    auto window = std::make_unique<SDLOpenGLWindow>(size);
    auto context = window->GetGraphicContext();
    if (!context)
    {
        return nullptr;
    }
    window->SetUniqueDevice(std::make_unique<Device>(context, size));
    return window;
}

std::unique_ptr<WindowInterface> CreateSDLOpenGLNone(glm::uvec2 size)
{
    auto window = std::make_unique<SDLOpenGLNone>(size);
    auto context = window->GetGraphicContext();
    if (!context)
    {
        return nullptr;
    }
    window->SetUniqueDevice(std::make_unique<Device>(context, size));
    return window;
}

} // End namespace frame::opengl.
