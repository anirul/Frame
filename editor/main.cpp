#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <chrono>

// From: https://sourceforge.net/p/predef/wiki/OperatingSystems/
#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "frame/common/application.h"
#include "frame/file/file_system.h"
#include "frame/file/image_stb.h"
#include "frame/gui/draw_gui_factory.h"
#include "frame/gui/window_logger.h"
#include "frame/gui/window_resolution.h"
#include "frame/json/parse_level.h"
#include "frame/logger.h"
#include <SDL3/SDL.h>
#include "absl/flags/flag.h"
#include "absl/flags/usage.h"
#include "menubar.h"
#include "menubar_file.h"
#include "menubar_view.h"
#include "window_start.h"
#include <filesystem>

namespace
{

constexpr glm::uvec2 kDefaultSize{1280u, 720u};

int Run(int argc, char** argv)
{
    absl::SetProgramUsageMessage(
        "FrameEditor --device={vulkan|opengl} "
        "[--auto_exit_seconds=<seconds>]");

    frame::common::Application app(argc, argv, kDefaultSize);
    const double auto_exit_seconds = absl::GetFlag(FLAGS_auto_exit_seconds);
    const auto run_start = std::chrono::steady_clock::now();
    bool auto_exit_triggered = false;
    auto& window = app.GetWindow();
    auto& device = window.GetDevice();
    auto gui_window = frame::gui::CreateDrawGui(window, {}, 20.0f);
    if (!gui_window)
    {
        throw std::runtime_error("Could not create GUI for selected backend.");
    }

    frame::gui::MenubarView menubar_view(
        device,
        *gui_window.get(),
        kDefaultSize,
        window.GetDesktopSize(),
        window.GetPixelPerInch());
    frame::gui::MenubarFile menubar_file(device, *gui_window.get(), "");
    gui_window->SetMenuBar(
        std::make_unique<frame::gui::Menubar>(
            "Menu", menubar_file, menubar_view, gui_window->GetDevice()));
    gui_window->AddWindow(std::make_unique<frame::gui::WindowStart>(menubar_file));
    // Set the main window in full.
    device.AddPlugin(std::move(gui_window));

    // Load a blank level so GUI elements can display before any project is
    // opened.
    app.Startup(
        frame::file::FindFile("asset/json/new_project_template.json"));

    bool loop = true;
    while (loop)
    {
        if (!menubar_file.GetFileName().empty())
        {
            if (device.GetDeviceEnum() == frame::RenderingAPIEnum::OPENGL)
            {
                device.Clear(glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
                SDL_GL_SwapWindow(
                    static_cast<SDL_Window*>(window.GetWindowContext()));
            }
            app.Startup(frame::file::FindFile(menubar_file.GetFileName()));
        }
        switch (app.Run(
            [&menubar_file,
             auto_exit_seconds,
             run_start,
             &auto_exit_triggered] {
                if (menubar_file.HasChanged())
                {
                    return false;
                }
                if (auto_exit_seconds <= 0.0)
                {
                    return true;
                }
                const auto now = std::chrono::steady_clock::now();
                const std::chrono::duration<double> elapsed = now - run_start;
                const bool keep_running = elapsed.count() < auto_exit_seconds;
                if (!keep_running && !auto_exit_triggered)
                {
                    auto& logger = frame::Logger::GetInstance();
                    logger->info(
                        "FrameEditor auto exit triggered after {:.3f} seconds.",
                        auto_exit_seconds);
                    logger->flush();
                    auto_exit_triggered = true;
                }
                return keep_running;
            }))
        {
        case frame::WindowReturnEnum::QUIT:
            return 0;
        case frame::WindowReturnEnum::RESTART:
            if (auto_exit_triggered)
            {
                return 0;
            }
            if (menubar_view.GetWindowResolution())
            {
                app.Resize(
                    menubar_view.GetWindowResolution()->GetSize(),
                    menubar_view.GetWindowResolution()->GetFullScreen());
                loop = menubar_view.GetWindowResolution()->End();
            }
            else
            {
                menubar_view.Reset();
                if (menubar_file.GetFileDialogEnum() ==
                    frame::gui::FileDialogEnum::NEW)
                {
                    std::filesystem::copy_file(
                        frame::file::FindFile(
                            "asset/json/new_project_template.json"),
                        menubar_file.GetFileName(),
                        std::filesystem::copy_options::overwrite_existing);
                }
                menubar_file.TrySaveFile();
            }
            break;
        default:
            frame::Logger::GetInstance()->warn(
                std::format("Weird window return enum?"));
            break;
        }
    }

    return 0;
}

} // namespace

// From: https://sourceforge.net/p/predef/wiki/OperatingSystems/
#if defined(_WIN32) || defined(_WIN64)
int WINAPI WinMain(
    _In_ HINSTANCE /*hInstance*/,
    _In_opt_ HINSTANCE /*hPrevInstance*/,
    _In_ LPSTR /*lpCmdLine*/,
    _In_ int /*nShowCmd*/)
#else
int main(int argc, char** argv)
#endif
try
{
#if defined(_WIN32) || defined(_WIN64)
    return Run(__argc, __argv);
#else
    return Run(argc, argv);
#endif
}
catch (const std::exception& ex)
{
#if defined(_WIN32) || defined(_WIN64)
    MessageBox(nullptr, ex.what(), "Exception", MB_ICONEXCLAMATION);
#else
    std::cerr << "Error: " << ex.what() << std::endl;
#endif
    return -2;
}
