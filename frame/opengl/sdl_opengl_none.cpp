#include "frame/opengl/sdl_opengl_none.h"

#include "frame/opengl/message_callback.h"
#include <SDL3/SDL.h>
#include <format>

namespace frame::opengl
{

SDLOpenGLNone::SDLOpenGLNone(glm::uvec2 size) : size_(size)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        throw std::runtime_error(
            std::format("Couldn't initialize SDL3: {}", SDL_GetError()));
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
#if defined(_WIN32) || defined(_WIN64)
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
#endif
#if defined(__linux__)
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
#endif

    sdl_window_ = SDL_CreateWindow(
        "SDL OpenGL", size_.x, size_.y, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!sdl_window_)
    {
        throw std::runtime_error(
            std::format("Couldn't start a window in SDL3: {}", SDL_GetError()));
    }
    logger_->info("Created an SDL3 window.");

    // Now create GL context
    gl_context_ = SDL_GL_CreateContext(sdl_window_);
    if (!gl_context_)
    {
        throw std::runtime_error(
            std::format("Failed to create GL context: {}", SDL_GetError()));
    }

    // Set as current
    SDL_GL_MakeCurrent(sdl_window_, gl_context_);
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        throw std::runtime_error("glad load failed");
    }
    SDL_GL_SetSwapInterval(0);
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, nullptr);
}

SDLOpenGLNone::~SDLOpenGLNone()
{
    if (gl_context_)
    {
        SDL_GL_DestroyContext(gl_context_);
        gl_context_ = nullptr;
    }
    if (sdl_window_)
    {
        SDL_DestroyWindow(sdl_window_);
        sdl_window_ = nullptr;
    }
    SDL_Quit();
}

WindowReturnEnum SDLOpenGLNone::Run(std::function<bool()> lambda)
{
    for (const auto& plugin_interface : device_->GetPluginPtrs())
    {
        plugin_interface->Startup(size_);
    }
    if (input_interface_)
        input_interface_->NextFrame();
    device_->Display(0.0);
    for (const auto& plugin_interface : device_->GetPluginPtrs())
    {
        plugin_interface->Update(*device_.get(), 0.0);
    }
    lambda();
    return WindowReturnEnum::UKNOWN;
}

void* SDLOpenGLNone::GetGraphicContext() const
{
    std::pair<int, int> gl_version;
    frame::Logger& logger = frame::Logger::GetInstance();

    // GL context.
    // Re-bind the context to the calling thread
    if (!SDL_GL_MakeCurrent(sdl_window_, gl_context_))
    {
        throw std::runtime_error(
            std::format("MakeCurrent failed: {}", SDL_GetError()));
    }

    const char* ver = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (!ver)
    {
        throw std::runtime_error("No current GL context");
    }

    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &gl_version.first);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &gl_version.second);
    logger->info(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    logger->info(
        "Started SDL OpenGL version {}.{}.",
        gl_version.first,
        gl_version.second);

    return gl_context_;
}

} // End namespace frame::opengl.
