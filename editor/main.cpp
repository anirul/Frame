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
#include "menubar.h"
#include "menubar_view.h"

// From: https://sourceforge.net/p/predef/wiki/OperatingSystems/
#if defined(_WIN32) || defined(_WIN64)
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
#else
int main(int ac, char** av)
#endif
try
{
    glm::uvec2 size = {1280, 720};
    bool end = true;
    auto win = frame::CreateNewWindow(
        frame::DrawingTargetEnum::WINDOW,
        frame::RenderingAPIEnum::OPENGL,
        size);
    auto& device = win->GetDevice();
    auto gui_window = frame::gui::CreateDrawGui(*win.get(), {}, 20.0f);
    frame::gui::MenubarView menubar_view(
        gui_window.get(), size, win->GetDesktopSize(), win->GetPixelPerInch());
    gui_window->SetMenuBar(
        std::make_unique<frame::gui::Menubar>(
            "Menu", menubar_view, gui_window->GetDevice()));
    // Set the main window in full.
    win->GetDevice().AddPlugin(std::move(gui_window));
    frame::common::Application app(std::move(win));
    do
    {
        app.Startup(frame::file::FindFile("asset/json/editor.json"));
        app.Run();
        if (menubar_view.GetWindowResolution())
        {
            app.Resize(
                menubar_view.GetWindowResolution()->GetSize(),
                menubar_view.GetWindowResolution()->GetFullScreen());
        }
    } while (menubar_view.GetWindowResolution() &&
             !menubar_view.GetWindowResolution()->End());
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
