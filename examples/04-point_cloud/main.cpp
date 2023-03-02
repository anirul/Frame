#include <absl/flags/flag.h>
#include <absl/flags/parse.h>

#include <iostream>
#include <utility>
#include <vector>

// From: https://sourceforge.net/p/predef/wiki/OperatingSystems/
#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "frame/common/application.h"
#include "frame/file/file_system.h"
#include "frame/file/image_stb.h"
#include "frame/gui/draw_gui_factory.h"
#include "frame/gui/input_factory.h"
#include "frame/gui/window_camera.h"
#include "frame/gui/window_resolution.h"
#include "frame/json/parse_level.h"
#include "frame/window_factory.h"

ABSL_FLAG(float, move_mult, 1.0f, "Move multiplication factor.");
ABSL_FLAG(float, zoom_mult, 1.0f, "Zoom multiplication factor.");

// From: https://sourceforge.net/p/predef/wiki/OperatingSystems/
#if defined(_WIN32) || defined(_WIN64)
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine,
                   _In_ int nShowCmd) try {
    int ac    = __argc;
    char** av = __argv;
#else
int main(int ac, char** av) try {
#endif
    absl::ParseCommandLine(ac, av);
    glm::uvec2 size = { 800, 600 };
    auto win        = frame::CreateNewWindow(frame::DrawingTargetEnum::WINDOW,
                                             frame::RenderingAPIEnum::OPENGL, size);
    frame::gui::WindowResolution* ptr_window_resolution = nullptr;
    frame::gui::WindowCamera* ptr_window_camera         = nullptr;
    auto gui_window = frame::gui::CreateDrawGui(win->GetUniqueDevice(), *win.get());
    {
        auto gui_resolution = std::make_unique<frame::gui::WindowResolution>(
            "Resolution", size, win->GetDesktopSize(), win->GetPixelPerInch());
        ptr_window_resolution = gui_resolution.get();
        gui_window->AddWindow(std::move(gui_resolution));
    }
    {
        auto gui_camera   = std::make_unique<frame::gui::WindowCamera>("Camera");
        ptr_window_camera = gui_camera.get();
        gui_window->AddWindow(std::move(gui_camera));
    }
    win->GetUniqueDevice().AddPlugin(std::move(gui_window));
    win->SetInputInterface(frame::gui::CreateInputArcball(
        win->GetUniqueDevice(), glm::vec3(0, 0, 1.0f), absl::GetFlag(FLAGS_move_mult),
        absl::GetFlag(FLAGS_zoom_mult)));
    auto& device_ref = win->GetUniqueDevice();
    frame::common::Application app(std::move(win));
    std::vector<bool> check_end;
    bool do_once = true;
    do {
        std::unique_ptr<frame::LevelInterface> level = frame::proto::ParseLevel(
            size, frame::file::FindFile("asset/json/point_cloud.json"));
        // All except first.
        if (!std::exchange(do_once, false)) {
            level->GetDefaultCamera().operator=(ptr_window_camera->GetSavedCamera());
        }
        ptr_window_camera->SetCameraPtr(&level->GetDefaultCamera());
        app.Startup(std::move(level));
        app.Run();
        app.Resize(ptr_window_resolution->GetSize(), ptr_window_resolution->GetFullScreen());
        ptr_window_camera->SaveCamera();
        check_end = { ptr_window_resolution->End(), ptr_window_camera->End() };
    } while (!std::all_of(check_end.begin(), check_end.end(), [](bool b) { return b; }));
    return 0;
} catch (std::exception ex) {
    // From: https://sourceforge.net/p/predef/wiki/OperatingSystems/
#if defined(_WIN32) || defined(_WIN64)
    MessageBox(nullptr, ex.what(), "Exception", MB_ICONEXCLAMATION);
#else
    std::cerr << "Error: " << ex.what() << std::endl;
#endif
    return -2;
}
