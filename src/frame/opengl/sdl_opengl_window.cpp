#include "frame/opengl/sdl_opengl_window.h"

// Needed under windows to get the PPI.
#if defined(_WIN32) || defined(_WIN64)
#define NOMIXMAX
#include <shellscalingapi.h>
#include <shtypes.h>
#pragma comment(lib, "Shcore.lib")
#endif
#include <SDL3/SDL_video.h>
#include <format>

#include "frame/gui/draw_gui_interface.h"
#include "frame/opengl/gui/sdl_opengl_draw_gui.h"
#include "frame/opengl/message_callback.h"

namespace frame::opengl
{

SDLOpenGLWindow::SDLOpenGLWindow(glm::uvec2 size) : size_(size)
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
        "SDL OpenGL",
        size_.x,
        size_.y,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (!sdl_window_)
    {
        throw std::runtime_error("Couldn't start a window in SDL3.");
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

#if defined(_WIN32) || defined(_WIN64)
    // Get the window handler.
    hwnd_ = FindWindowA(nullptr, SDL_GetWindowTitle(sdl_window_));
#endif
    // Query the desktop size, used in full screen desktop mode.
    int w;
    int h;
    SDL_GetWindowSize(sdl_window_, &w, &h);
    desktop_size_.x = w;
    desktop_size_.y = h;

    // Vsync off.
    SDL_GL_SetSwapInterval(0);

    // Initialize GLEW to find the 'glDebugMessageCallback' function.
    glewExperimental = GL_TRUE;
    auto result = glewInit();
    if (result != GLEW_OK)
    {
        throw std::runtime_error(
            std::format(
                "GLEW problems : {}",
                reinterpret_cast<const char*>(glewGetErrorString(result))));
    }

    // During init, enable debug output
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, nullptr);
}

SDLOpenGLWindow::~SDLOpenGLWindow()
{
    // TODO(anirul): Fix me to check which device this is.
    if (device_)
    {
        SDL_GL_DestroyContext(
            static_cast<SDL_GLContext>(device_->GetDeviceContext()));
    }
    SDL_DestroyWindow(sdl_window_);
    SDL_Quit();
}

WindowReturnEnum SDLOpenGLWindow::Run(std::function<bool()> lambda)
{
    // Called only once at the beginning.
    for (const auto& plugin_interface : device_->GetPluginPtrs())
    {
        // In case this is a removed one it will be nulled.
        if (plugin_interface)
        {
            // This will call the device startup.
            plugin_interface->Startup(size_);
        }
    }
    WindowReturnEnum window_return_enum = WindowReturnEnum::CONTINUE;
    double previous_count = 0.0;
    // Timing counter.
    auto start = std::chrono::system_clock::now();
    while (window_return_enum == WindowReturnEnum::CONTINUE)
    {
        // Compute the time difference from previous frame.
        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> time = end - start;
        const double dt = GetFrameDt(time.count());

        // Process events.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (!RunEvent(event, dt))
            {
                window_return_enum = WindowReturnEnum::QUIT;
            }
            for (const auto& plugin_interface : device_->GetPluginPtrs())
            {
                if (plugin_interface)
                {
                    plugin_interface->PollEvent(&event);
                }
            }
        }
        if (input_interface_)
        {
            input_interface_->NextFrame();
        }
        device_->Display(time.count());

        // Draw the Scene not used?
        for (const auto& plugin_interface : device_->GetPluginPtrs())
        {
            if (plugin_interface)
            {
                if (!plugin_interface->Update(*device_.get(), time.count()))
                {
                    window_return_enum = WindowReturnEnum::QUIT;
                }
            }
        }

        SDL_GL_MakeCurrent(sdl_window_, gl_context_);

        SetWindowTitle(
            "SDL OpenGL - " + std::to_string(static_cast<float>(GetFPS(dt))));
        previous_count = time.count();
        if (!lambda())
        {
            window_return_enum = WindowReturnEnum::RESTART;
        }

        // TODO(anirul): Fix me to check which device this is.
        if (device_)
        {
            SDL_GL_SwapWindow(sdl_window_);
        }
    }
    return window_return_enum;
}

bool SDLOpenGLWindow::RunEvent(const SDL_Event& event, const double dt)
{
    if (event.type == SDL_EVENT_QUIT)
    {
        return false;
    }
    if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED)
    {
        SDL_StartTextInput(sdl_window_);
    }
    if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST)
    {
        SDL_StopTextInput(sdl_window_);
    }
    bool has_window_plugin = false;
    for (PluginInterface* plugin : device_->GetPluginPtrs())
    {
        if (dynamic_cast<frame::gui::DrawGuiInterface*>(plugin))
        {
            has_window_plugin = true;
        }
    }
    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        switch (event.key.key)
        {
        case SDLK_F11:
            if (has_window_plugin)
            {
                for (PluginInterface* plugin : device_->GetPluginPtrs())
                {
                    auto* window_plugin =
                        dynamic_cast<frame::gui::DrawGuiInterface*>(plugin);
                    if (window_plugin)
                    {
                        auto is_visible = window_plugin->IsVisible();
                        window_plugin->SetVisible(!is_visible);
                    }
                }
                return true;
            }
            break;
        case SDLK_PRINTSCREEN:
            device_->ScreenShot("ScreenShot.png");
            return true;
        default:
            if (key_callbacks_.count(event.key.key))
            {
                return key_callbacks_[event.key.key]();
            }
            break;
        }
    }
    if (input_interface_)
    {
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            return input_interface_->KeyPressed(event.key.key, dt);
        }
        if (event.type == SDL_EVENT_KEY_UP)
        {
            return input_interface_->KeyReleased(event.key.key, dt);
        }
        if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            return input_interface_->MouseMoved(
                glm::vec2(event.motion.x, event.motion.y),
                glm::vec2(event.motion.xrel, event.motion.yrel),
                dt);
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            return input_interface_->MousePressed(
                SDLButtonToChar(event.button.button), dt);
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            return input_interface_->MouseReleased(
                SDLButtonToChar(event.button.button), dt);
        }
        if (event.type == SDL_EVENT_MOUSE_WHEEL && event.wheel.y != 0)
        {
            return input_interface_->WheelMoved(
                static_cast<float>(event.wheel.y), dt);
        }
    }
    return true;
}

const char SDLOpenGLWindow::SDLButtonToChar(const Uint8 button) const
{
    char ret = 0;
    if (button & SDL_BUTTON_LEFT)
    {
        ret += 1;
    }
    if (button & SDL_BUTTON_RIGHT)
    {
        ret += 2;
    }
    if (button & SDL_BUTTON_MIDDLE)
    {
        ret += 4;
    }
    if (button & SDL_BUTTON_X1)
    {
        ret += 8;
    }
    if (button & SDL_BUTTON_X2)
    {
        ret += 16;
    }
    return ret;
}

const double SDLOpenGLWindow::GetFrameDt(const double t) const
{
    static double previous_t = 0.0;
    double ret = t - previous_t;
    previous_t = t;
    return ret;
}

void* SDLOpenGLWindow::GetGraphicContext() const
{
    std::pair<int, int> gl_version;
    frame::Logger& logger = frame::Logger::GetInstance();

    // GL context.
    if (!gl_context_)
    {
        std::string error = SDL_GetError();
        logger->error(error);
        throw std::runtime_error(error);
    }

    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &gl_version.first);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &gl_version.second);
    static bool s_once = false;
    if (!s_once)
    {
        s_once = true;
        logger->info(
            "OpenGL version: {}.{}", gl_version.first, gl_version.second);
    }

    return gl_context_;
}

void SDLOpenGLWindow::Resize(glm::uvec2 size, FullScreenEnum fullscreen_enum)
{
    size_ = size;
    if (fullscreen_enum_ != fullscreen_enum)
    {
        fullscreen_enum_ = fullscreen_enum;

        const SDL_DisplayMode* mode_ptr = nullptr;
        SDL_DisplayMode fullscreen_mode{};

        if (fullscreen_enum_ == FullScreenEnum::WINDOW)
        {
            mode_ptr = nullptr;
        }
        else if (fullscreen_enum_ == FullScreenEnum::FULLSCREEN)
        {
            SDL_DisplayID display = SDL_GetDisplayForWindow(sdl_window_);
            if (!display)
            {
                throw std::runtime_error(
                    std::format(
                        "SDL_GetDisplayForWindow failed: {}", SDL_GetError()));
            }

            SDL_Rect bounds;
            if (!SDL_GetDisplayBounds(display, &bounds))
            {
                throw std::runtime_error(
                    std::format(
                        "SDL_GetDisplayBounds failed: {}", SDL_GetError()));
            }

            // Use desired resolution if needed
            fullscreen_mode.w = bounds.w;
            fullscreen_mode.h = bounds.h;
            mode_ptr = &fullscreen_mode;
        }
        else if (fullscreen_enum_ == FullScreenEnum::FULLSCREEN_DESKTOP)
        {
            fullscreen_mode.w = 0;
            fullscreen_mode.h = 0;
            fullscreen_mode.refresh_rate = 0;
            mode_ptr = &fullscreen_mode;
        }

        if (!SDL_SetWindowFullscreenMode(sdl_window_, mode_ptr))
        {
            throw std::runtime_error(
                std::format(
                    "Error switching fullscreen mode: {}", SDL_GetError()));
        }

        // Only resize in windowed mode — fullscreen modes will auto-resize the
        // window
        if (fullscreen_enum_ == FullScreenEnum::WINDOW)
        {
            SDL_SetWindowSize(sdl_window_, size_.x, size_.y);
        }
    }

    device_->Resize(size_);
}

frame::FullScreenEnum SDLOpenGLWindow::GetFullScreenEnum() const
{
    return fullscreen_enum_;
}

#if defined(_WIN32) || defined(_WIN64)
namespace
{

std::vector<glm::vec2> s_ppi_vec = {};

// This is a callback to receive the monitor and then to compute the PPI.
BOOL MonitorEnumProc(HMONITOR hmonitor, HDC hdc, LPRECT p_rect, LPARAM param)
{
    std::uint32_t hppi = 0;
    std::uint32_t vppi = 0;

    // Get the PPI from monitor use the raw DPI to get real values not
    // the one from font.
    if (GetDpiForMonitor(hmonitor, MDT_RAW_DPI, &hppi, &vppi) != S_OK)
    {
        throw std::runtime_error("Couldn't get the PPI.");
    }

    // Get the logical width and height of the monitor.
    MONITORINFOEX miex;
    miex.cbSize = sizeof(miex);
    GetMonitorInfo(hmonitor, &miex);
    int cx_logical = (miex.rcMonitor.right - miex.rcMonitor.left);
    int cy_logical = (miex.rcMonitor.bottom - miex.rcMonitor.top);

    // Get the physical width and height of the monitor.
    DEVMODE dm;
    dm.dmSize = sizeof(dm);
    dm.dmDriverExtra = 0;
    EnumDisplaySettings(miex.szDevice, ENUM_CURRENT_SETTINGS, &dm);
    int cx_physical = dm.dmPelsWidth;
    int cy_physical = dm.dmPelsHeight;

    // Calculate the scaling factor.
    float horz_scale =
        static_cast<float>(cx_physical) / static_cast<float>(cx_logical);
    float vert_scale =
        static_cast<float>(cy_physical) / static_cast<float>(cy_logical);
    // Used a epsilon to avoid problems in the assertion.
    assert(fabs(horz_scale - vert_scale) < 1e-3f);
    float multiplication_scale = horz_scale;

    // Push back the value to the vector.
    s_ppi_vec.push_back(
        glm::vec2(
            static_cast<float>(hppi) * multiplication_scale,
            static_cast<float>(vppi) * multiplication_scale));
    return TRUE;
}

} // End namespace.
#endif

glm::vec2 SDLOpenGLWindow::GetPixelPerInch(std::uint32_t screen /*= 0*/) const
{
#if defined(_WIN32) || defined(_WIN64)
    // Reset the vector.
    s_ppi_vec = {};
    // Enumerate the monitor with a callback to push the ppi in the vector.
    if (!EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, 0))
    {
        throw std::runtime_error("Couldn't enumerate monitors.");
    }
    // Check if we tried to get a screen that is in the screen values.
    if (s_ppi_vec.size() < screen)
    {
        throw std::runtime_error("Outside screen.");
    }
    return s_ppi_vec[screen];
#else // Not windows.
    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    float scale = SDL_GetDisplayContentScale(display);
    if (scale != 0.0f)
    {
        float dpi = scale * 96.0f; // 96 is the base DPI on many desktop systems
        return glm::vec2(dpi, dpi);
    }
    else
    {
        throw std::runtime_error("Error couldn't get the DPI");
    }
#endif
}

} // End namespace frame::opengl.
