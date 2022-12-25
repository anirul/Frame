#include "frame/opengl/sdl_opengl_none.h"

#include <fmt/format.h>

#include "frame/opengl/message_callback.h"

namespace frame::opengl {

SDLOpenGLNone::SDLOpenGLNone(glm::uvec2 size) : size_(size) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) != 0) {
        throw std::runtime_error(fmt::format("Couldn't initialize SDL2: {}", SDL_GetError()));
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    // CHECKME(anirul): Is this still relevant?
#if defined(__APPLE__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif
#if defined(_WIN32) || defined(_WIN64)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
#endif
#if defined(__linux__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
#endif

    sdl_window_ = SDL_CreateWindow("SDL OpenGL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                   size_.x, size_.y, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!sdl_window_) {
        throw std::runtime_error(
            fmt::format("Couldn't start a window in SDL2: {}", SDL_GetError()));
    }
    logger_->info("Created an SDL2 window.");
}

SDLOpenGLNone::~SDLOpenGLNone() {
    // TODO(anirul): Fix me to check which device this is.
    if (device_) SDL_GL_DeleteContext(device_->GetDeviceContext());
    SDL_DestroyWindow(sdl_window_);
    SDL_Quit();
}

void SDLOpenGLNone::Run() {
    for (const auto& plugin_interface : device_->GetPluginPtrs()) {
        plugin_interface->Startup(size_);
    }
    if (input_interface_) input_interface_->NextFrame();
    device_->Display(0.0);
    for (const auto& plugin_interface : device_->GetPluginPtrs()) {
        plugin_interface->Update(device_.get(), 0.0);
    }
}

void* SDLOpenGLNone::GetGraphicContext() const {
    std::pair<int, int> gl_version;
    frame::Logger& logger = frame::Logger::GetInstance();

    // GL context.
    void* gl_context = SDL_GL_CreateContext(static_cast<SDL_Window*>(GetWindowContext()));
    if (!gl_context) {
        std::string error = SDL_GetError();
        logger->error(error);
        throw std::runtime_error(error);
    }

    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &gl_version.first);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &gl_version.second);
    logger->info(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    // Vsync off.
    SDL_GL_SetSwapInterval(0);

    logger->info("Started SDL OpenGL version {}.{}.", gl_version.first, gl_version.second);

    // Initialize GLEW to find the 'glDebugMessageCallback' function.
    auto result = glewInit();
    if (result != GLEW_OK) {
        throw std::runtime_error(fmt::format(
            "GLEW problems : {}", reinterpret_cast<const char*>(glewGetErrorString(result))));
    }

    // During init, enable debug output
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, nullptr);

    return gl_context;
}

}  // End namespace frame::opengl.
