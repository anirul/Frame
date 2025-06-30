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
#include "frame/json/parse_level.h"
#include "frame/window_factory.h"
#include "menubar.h"
#include "menubar_file.h"
#include "menubar_view.h"
#include "window_start.h"
#include <filesystem>

//! Minimal blank level used so the editor can start without loading a project.
static constexpr std::string_view kBlankLevelJson = R"({
  "name": "Blank",
  "default_texture_name": "DefaultTexture",
  "textures": [
    {
      "name": "DefaultTexture",
      "size": { "x": -1, "y": -1 },
      "pixel_element_size": { "value": "BYTE" },
      "pixel_structure": { "value": "RGB" }
    }
  ],
  "programs": [],
  "scene_tree": {
    "default_root_name": "root",
    "default_camera_name": "camera",
    "node_matrices": [
      { "name": "root", "quaternion": { "w": 1, "x": 0, "y": 0, "z": 0 } }
    ],
    "node_cameras": [
      {
        "name": "camera",
        "parent": "root",
        "position": { "x": 0, "y": 0, "z": 0 },
        "target": { "x": 0, "y": 0, "z": 1 },
        "up": { "x": 0, "y": 1, "z": 0 },
        "fov_degrees": 65.0,
        "aspect_ratio": 1.6,
        "near_clip": 0.1,
        "far_clip": 10000.0
      }
    ]
  },
  "materials": []
})";

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
    auto win = frame::CreateNewWindow(
        frame::DrawingTargetEnum::WINDOW,
        frame::RenderingAPIEnum::OPENGL,
        size);
    auto& device = win->GetDevice();
    auto gui_window = frame::gui::CreateDrawGui(*win.get(), {}, 20.0f);
    frame::gui::MenubarView menubar_view(
        device,
        *gui_window.get(),
        size,
        win->GetDesktopSize(),
        win->GetPixelPerInch());
    frame::gui::MenubarFile menubar_file(device, *gui_window.get(), "");
    gui_window->SetMenuBar(
        std::make_unique<frame::gui::Menubar>(
            "Menu", menubar_file, menubar_view, gui_window->GetDevice()));
    gui_window->AddModalWindow(
        std::make_unique<frame::gui::WindowStart>(menubar_file));
    // Set the main window in full.
    device.AddPlugin(std::move(gui_window));
    frame::common::Application app(std::move(win));
    // Load a blank level so GUI elements can display before any project is
    // opened.
    app.Startup(frame::json::ParseLevel(size, std::string(kBlankLevelJson)));
    bool loop = true;
    while (loop)
    {
        if (!menubar_file.GetFileName().empty())
        {
            app.Startup(frame::file::FindFile(menubar_file.GetFileName()));
        }
        switch (app.Run([&menubar_file] { return !menubar_file.HasChanged(); }))
        {
        case frame::WindowReturnEnum::QUIT:
            return 0;
        case frame::WindowReturnEnum::RESTART:
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
catch (const std::exception& ex)
{
#if defined(_WIN32) || defined(_WIN64)
    MessageBox(nullptr, ex.what(), "Exception", MB_ICONEXCLAMATION);
#else
    std::cerr << "Error: " << ex.what() << std::endl;
#endif
    return -2;
}
