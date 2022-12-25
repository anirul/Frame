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
#include "frame/file/image_stb.h"
#include "frame/gui/draw_gui_factory.h"
#include "frame/gui/input_wasd_factory.h"
#include "frame/gui/window_resolution.h"
#include "frame/window_factory.h"

ABSL_FLAG(float, move_mult, 0.1f, "Move multiplication factor.");
ABSL_FLAG(float, rotation_mult, 0.25f, "Rotation multiplication factor.");

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
    auto gui_window = frame::gui::CreateDrawGui(win->GetUniqueDevice(), win.get());
    auto gui_resolution =
        std::make_unique<frame::gui::WindowResolution>("Resolution", size, win->GetDesktopSize());
    ptr_window_resolution = gui_resolution.get();
    gui_window->AddWindow(std::move(gui_resolution));
    win->GetUniqueDevice()->AddPlugin(std::move(gui_window));
    win->SetInputInterface(frame::gui::CreateInputWasd(win->GetUniqueDevice(),
                                                       absl::GetFlag(FLAGS_move_mult),
                                                       absl::GetFlag(FLAGS_rotation_mult)));
    frame::common::Application app(std::move(win));
    do {
        app.Startup(frame::file::FindFile("asset/json/point_cloud.json"));
        app.Run();
        app.Resize(ptr_window_resolution->GetSize(), ptr_window_resolution->GetFullScreen());
    } while (!ptr_window_resolution->End());
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
