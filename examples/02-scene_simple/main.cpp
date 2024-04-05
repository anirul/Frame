#include <iostream>
#include <string>
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
#include "frame/gui/window_logger.h"
#include "frame/gui/window_resolution.h"
#include "frame/window_factory.h"
#include "modal_info.h"

// From: https://sourceforge.net/p/predef/wiki/OperatingSystems/
#if defined(_WIN32) || defined(_WIN64)
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
try
{
#else
int main(int ac, char** av)
try
{
#endif
    glm::uvec2 size = {1280, 720};
    bool end = true;

    frame::gui::WindowResolution* ptr_window_resolution = nullptr;
    auto win = frame::CreateNewWindow(
        frame::DrawingTargetEnum::WINDOW,
        frame::RenderingAPIEnum::OPENGL,
        size);
    auto& device = win->GetDevice();
    auto gui_window = frame::gui::CreateDrawGui(*win.get(), {}, 20.0f);
    auto gui_resolution = std::make_unique<frame::gui::WindowResolution>(
        "Resolution", size, win->GetDesktopSize(), win->GetPixelPerInch());
    ptr_window_resolution = gui_resolution.get();
    gui_window->AddWindow(std::move(gui_resolution));
    gui_window->AddWindow(std::make_unique<frame::gui::WindowLogger>("Logger"));
    // Set the main window in full.
    // gui_window->SetVisible(false);
    gui_window->AddModalWindow(
        std::make_unique<ModalInfo>("Info", "This is a test modal window."));
    win->GetDevice().AddPlugin(std::move(gui_window));
    frame::common::Application app(std::move(win));
    do
    {
        app.Startup(frame::file::FindFile("asset/json/scene_simple.json"));
        app.Run();
        app.Resize(
            ptr_window_resolution->GetSize(),
            ptr_window_resolution->GetFullScreen());
        device.SetStereo(
            ptr_window_resolution->GetStereo(),
            ptr_window_resolution->GetInterocularDistance(),
            ptr_window_resolution->GetFocusPoint(),
            ptr_window_resolution->IsInvertLeftRight());
    } while (!ptr_window_resolution->End());
    return 0;
}
catch (std::exception ex)
{
#if defined(_WIN32) || defined(_WIN64)
    MessageBox(nullptr, ex.what(), "Exception", MB_ICONEXCLAMATION);
#else
    std::cerr << "Error: " << ex.what() << std::endl;
#endif
    return -2;
}
