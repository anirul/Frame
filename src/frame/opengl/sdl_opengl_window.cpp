#include "frame/opengl/sdl_opengl_window.h"

#include "frame/opengl/message_callback.h"

namespace frame::opengl {

SDLOpenGLWindow::SDLOpenGLWindow(std::pair<std::uint32_t, std::uint32_t> size) : size_(size) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) throw std::runtime_error("Couldn't initialize SDL2.");
    sdl_window_ = SDL_CreateWindow("SDL OpenGL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   size_.first, size_.second, SDL_WINDOW_OPENGL);
    if (!sdl_window_) {
        throw std::runtime_error("Couldn't start a window in SDL2.");
    }
    logger_->info("Created an SDL2 window.");
#if defined(_WIN32) || defined(_WIN64)
    // Get the window handler.
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(sdl_window_, &wmInfo);
    hwnd_ = wmInfo.info.win.window;
#endif
    // Query the desktop size, used in full screen desktop mode.
    int i = SDL_GetWindowDisplayIndex(sdl_window_);
    SDL_Rect j;
    SDL_GetDisplayBounds(i, &j);
    desktop_size_.first  = j.w;
    desktop_size_.second = j.h;
}

SDLOpenGLWindow::~SDLOpenGLWindow() {
    // TODO(anirul): Fix me to check which device this is.
    if (device_) SDL_GL_DeleteContext(device_->GetDeviceContext());
    SDL_DestroyWindow(sdl_window_);
    SDL_Quit();
}

void SDLOpenGLWindow::Run() {
    // Called only once at the beginning.
    for (const auto& draw_interface : draw_interfaces_) {
        // In case this is a removed one it will be nulled.
        if (draw_interface) {
            // This will call the device startup.
            draw_interface->Startup(size_);
        }
    }
    // While Run return true continue.
    bool loop             = true;
    double previous_count = 0.0;
    // Timing counter.
    auto start = std::chrono::system_clock::now();
    do {
        // Compute the time difference from previous frame.
        auto end                           = std::chrono::system_clock::now();
        std::chrono::duration<double> time = end - start;
        const double dt                    = GetFrameDt(time.count());

        // Process events.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            bool skip = false;
            for (const auto& draw_interface : draw_interfaces_) {
                if (draw_interface) {
                    if (draw_interface->PollEvent(&event)) skip = true;
                }
            }
            if (skip) continue;
            if (!RunEvent(event, dt)) {
                loop = false;
            }     
        }
        if (input_interface_) input_interface_->NextFrame();

        device_->Display(time.count());

        // Draw the Scene not used?
        for (const auto& draw_interface : draw_interfaces_) {
            if (draw_interface) {
                if (!draw_interface->RunDraw(time.count())) {
                    loop = false;
                }
            }
        }

        SetWindowTitle("SDL OpenGL - " + std::to_string(static_cast<float>(GetFPS(dt))));

        previous_count = time.count();

        // TODO(anirul): Fix me to check which device this is.
        if (device_) SDL_GL_SwapWindow(sdl_window_);
    } while (loop);
}

bool SDLOpenGLWindow::RunEvent(const SDL_Event& event, const double dt) {
    if (event.type == SDL_QUIT) return false;
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
                return false;
            case SDLK_PRINTSCREEN:
                device_->ScreenShot("ScreenShot.png");
                return true;
        }
    }
    if (input_interface_) {
        if (event.type == SDL_KEYDOWN) {
            return input_interface_->KeyPressed(event.key.keysym.sym, dt);
        }
        if (event.type == SDL_KEYUP) {
            return input_interface_->KeyReleased(event.key.keysym.sym, dt);
        }
        if (event.type == SDL_MOUSEMOTION) {
            return input_interface_->MouseMoved(glm::vec2(event.motion.x, event.motion.y),
                                                glm::vec2(event.motion.xrel, event.motion.yrel),
                                                dt);
        }
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            return input_interface_->MousePressed(SDLButtonToChar(event.button.button), dt);
        }
        if (event.type == SDL_MOUSEBUTTONUP) {
            return input_interface_->MouseReleased(SDLButtonToChar(event.button.button), dt);
        }
    }
    return true;
}

const char SDLOpenGLWindow::SDLButtonToChar(const Uint8 button) const {
    char ret = 0;
    if (button & SDL_BUTTON_LEFT) ret += 1;
    if (button & SDL_BUTTON_RIGHT) ret += 2;
    if (button & SDL_BUTTON_MIDDLE) ret += 4;
    if (button & SDL_BUTTON_X1) ret += 8;
    if (button & SDL_BUTTON_X2) ret += 16;
    return ret;
}

const double SDLOpenGLWindow::GetFrameDt(const double t) const {
    static double previous_t = 0.0;
    double ret               = t - previous_t;
    previous_t               = t;
    return ret;
}

void* SDLOpenGLWindow::GetGraphicContext() const {
    std::pair<int, int> gl_version;
    frame::Logger& logger = frame::Logger::GetInstance();

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    // CHECKME(anirul): Is this still relevant?
#if defined(__APPLE__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
#endif

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

void SDLOpenGLWindow::Resize(std::pair<std::uint32_t, std::uint32_t> size) {
    size_ = size;
    SDL_SetWindowSize(sdl_window_, size_.first, size_.second);
    device_->Resize(size_);
}

void SDLOpenGLWindow::SetFullScreen(FullScreenEnum fullscreen_enum) {
    SDL_WindowFlags flags = static_cast<SDL_WindowFlags>(0);
    if (fullscreen_enum == FullScreenEnum::WINDOW) {
        flags = static_cast<SDL_WindowFlags>(0);
    }
    if (fullscreen_enum == FullScreenEnum::FULLSCREEN) {
        flags = SDL_WindowFlags::SDL_WINDOW_FULLSCREEN;
    }
    if (fullscreen_enum == FullScreenEnum::FULLSCREEN_DESKTOP) {
        flags = SDL_WindowFlags::SDL_WINDOW_FULLSCREEN_DESKTOP;
    }
    if (SDL_SetWindowFullscreen(sdl_window_, flags) < 0) {
        throw std::runtime_error(
            fmt::format("Error switching to fullscreen mode: {}", SDL_GetError()));
    }
}

frame::FullScreenEnum SDLOpenGLWindow::GetFullScreenEnum() const { return fullscreen_enum_; }

}  // End namespace frame::opengl.
