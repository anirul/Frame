#include "frame/opengl/sdl_opengl_window.h"

// Needed under windows to get the PPI.
#if defined(_WIN32) || defined(_WIN64)
#include <shellscalingapi.h>
#include <shtypes.h>
#pragma comment(lib, "Shcore.lib")
#endif

#include "frame/gui/draw_gui_interface.h"
#include "frame/opengl/gui/sdl_opengl_draw_gui.h"
#include "frame/opengl/message_callback.h"

namespace frame::opengl
{

SDLOpenGLWindow::SDLOpenGLWindow(glm::uvec2 size) : size_(size)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        throw std::runtime_error("Couldn't initialize SDL2.");
    sdl_window_ = SDL_CreateWindow(
        "SDL OpenGL",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        size_.x,
        size_.y,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (!sdl_window_)
    {
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
    desktop_size_.x = j.w;
    desktop_size_.y = j.h;
}

SDLOpenGLWindow::~SDLOpenGLWindow()
{
    // TODO(anirul): Fix me to check which device this is.
    if (device_)
        SDL_GL_DeleteContext(device_->GetDeviceContext());
    SDL_DestroyWindow(sdl_window_);
    SDL_Quit();
}

void SDLOpenGLWindow::Run(std::function<void()> lambda)
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
    // While Run return true continue.
    bool loop = true;
    double previous_count = 0.0;
    // Timing counter.
    auto start = std::chrono::system_clock::now();
    do
    {
        // Compute the time difference from previous frame.
        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> time = end - start;
        const double dt = GetFrameDt(time.count());

        // Process events.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            bool skip = false;
            for (const auto& plugin_interface : device_->GetPluginPtrs())
            {
                if (plugin_interface)
                {
                    if (plugin_interface->PollEvent(&event))
                        skip = true;
                }
            }
            if (skip)
            {
                continue;
            }
            if (!RunEvent(event, dt))
            {
                loop = false;
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
                    loop = false;
                }
            }
        }

        SetWindowTitle(
            "SDL OpenGL - " + std::to_string(static_cast<float>(GetFPS(dt))));
        previous_count = time.count();
        lambda();

        // TODO(anirul): Fix me to check which device this is.
        if (device_)
        {
            SDL_GL_SwapWindow(sdl_window_);
        }
    } while (loop);
}

bool SDLOpenGLWindow::RunEvent(const SDL_Event& event, const double dt)
{
    if (event.type == SDL_QUIT)
        return false;
    bool has_window_plugin = false;
    for (PluginInterface* plugin : device_->GetPluginPtrs())
    {
        if (dynamic_cast<frame::gui::DrawGuiInterface*>(plugin))
            has_window_plugin = true;
    }
    if (event.type == SDL_KEYDOWN)
    {
        if (key_callbacks_.count(event.key.keysym.sym))
        {
            return key_callbacks_[event.key.keysym.sym]();
        }
    }
    if (input_interface_)
    {
        if (event.type == SDL_KEYDOWN)
        {
            return input_interface_->KeyPressed(event.key.keysym.sym, dt);
        }
        if (event.type == SDL_KEYUP)
        {
            return input_interface_->KeyReleased(event.key.keysym.sym, dt);
        }
        if (event.type == SDL_MOUSEMOTION)
        {
            return input_interface_->MouseMoved(
                glm::vec2(event.motion.x, event.motion.y),
                glm::vec2(event.motion.xrel, event.motion.yrel),
                dt);
        }
        if (event.type == SDL_MOUSEBUTTONDOWN)
        {
            return input_interface_->MousePressed(
                SDLButtonToChar(event.button.button), dt);
        }
        if (event.type == SDL_MOUSEBUTTONUP)
        {
            return input_interface_->MouseReleased(
                SDLButtonToChar(event.button.button), dt);
        }
        if (event.type == SDL_MOUSEWHEEL && event.wheel.y != 0)
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
        ret += 1;
    if (button & SDL_BUTTON_RIGHT)
        ret += 2;
    if (button & SDL_BUTTON_MIDDLE)
        ret += 4;
    if (button & SDL_BUTTON_X1)
        ret += 8;
    if (button & SDL_BUTTON_X2)
        ret += 16;
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

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    // CHECKME(anirul): Is this still relevant?
#if defined(__APPLE__)
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif
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
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif

    // GL context.
    void* gl_context =
        SDL_GL_CreateContext(static_cast<SDL_Window*>(GetWindowContext()));
    if (!gl_context)
    {
        std::string error = SDL_GetError();
        logger->error(error);
        throw std::runtime_error(error);
    }

    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &gl_version.first);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &gl_version.second);
    logger->info(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    // Vsync off.
    SDL_GL_SetSwapInterval(0);

    logger->info(
        "Started SDL OpenGL version {}.{}.",
        gl_version.first,
        gl_version.second);

    // Initialize GLEW to find the 'glDebugMessageCallback' function.
    auto result = glewInit();
    if (result != GLEW_OK)
    {
        throw std::runtime_error(fmt::format(
            "GLEW problems : {}",
            reinterpret_cast<const char*>(glewGetErrorString(result))));
    }

    // During init, enable debug output
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, nullptr);

    return gl_context;
}

void SDLOpenGLWindow::Resize(glm::uvec2 size, FullScreenEnum fullscreen_enum)
{
    size_ = size;
    if (fullscreen_enum_ != fullscreen_enum)
    {
        fullscreen_enum_ = fullscreen_enum;
        SDL_WindowFlags flags = static_cast<SDL_WindowFlags>(0);
        if (fullscreen_enum_ == FullScreenEnum::WINDOW)
        {
            flags = static_cast<SDL_WindowFlags>(0);
        }
        if (fullscreen_enum_ == FullScreenEnum::FULLSCREEN)
        {
            flags = SDL_WindowFlags::SDL_WINDOW_FULLSCREEN;
        }
        if (fullscreen_enum_ == FullScreenEnum::FULLSCREEN_DESKTOP)
        {
            flags = SDL_WindowFlags::SDL_WINDOW_FULLSCREEN_DESKTOP;
        }
        // Change window mode.
        if (SDL_SetWindowFullscreen(sdl_window_, flags) < 0)
        {
            throw std::runtime_error(fmt::format(
                "Error switching to fullscreen mode: {}", SDL_GetError()));
        }
        // Only resize when changing window mode.
        SDL_SetWindowSize(sdl_window_, size_.x, size_.y);
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

// This is a callback to receive the monitor and then to compute the
// PPI.
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
    s_ppi_vec.push_back(glm::vec2(
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
    float hppi = 0.0f;
    float vppi = 0.0f;
    float dppi = 0.0f;
    // Use the SDL function this should work on Linux?
    if (SDL_GetDisplayDPI(screen, &dppi, &hppi, &vppi))
    {
        throw std::runtime_error(
            fmt::format("Error in GetPixelPerInch: {}", SDL_GetError()));
    }
    return glm::vec2(hppi, vppi);
#endif
}

} // End namespace frame::opengl.
