#include "menubar.h"

#include "frame/gui/window_file_dialog.h"
#include "frame/gui/window_glsl_file.h"
#include "frame/gui/window_logger.h"
#include "frame/gui/window_resolution.h"
#include "frame/logger.h"
#include "window_level.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <set>

namespace frame::gui
{

Menubar::Menubar(
    const std::string& name,
    MenubarFile& menubar_file,
    MenubarView& menubar_view,
    DeviceInterface& device)
    : name_(name), menubar_file_(menubar_file), menubar_view_(menubar_view),
      device_(device)
{
    SetName(name);
}

void Menubar::MenuFile()
{
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("New Project", "Ctrl+N"))
        {
            menubar_file_.ShowNewProject();
        }
        if (ImGui::MenuItem("Open Project", "Ctrl+O"))
        {
            menubar_file_.ShowOpenProject();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Save Project", "Ctrl+S"))
        {
            // TODO(anirul): Complete me!
        }
        if (ImGui::MenuItem("Save Project As...", "Ctrl+Shift+S"))
        {
            menubar_file_.ShowSaveAsProject();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit", "Alt+F4"))
        {
            // A little strong but it work.
            exit(0);
        }
        ImGui::EndMenu();
    }
}

void Menubar::MenuEdit()
{
    if (ImGui::BeginMenu("Edit"))
    {
        if (ImGui::MenuItem("Cut", "Ctrl+X"))
        {
        }
        if (ImGui::MenuItem("Copy", "Ctrl+C"))
        {
        }
        if (ImGui::MenuItem("Paste", "Ctrl+V"))
        {
        }
        if (ImGui::MenuItem("Delete", "Del"))
        {
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Level Editor"))
        {
            menubar_view_.GetDrawGui().AddWindow(
                std::make_unique<WindowLevel>(
                    device_,
                    menubar_view_.GetDrawGui(),
                    menubar_file_.GetFileName()));
        }
        if (ImGui::BeginMenu("Shader"))
        {
            std::set<std::string> shader_names;
            auto& level = device_.GetLevel();
            for (auto program_id : level.GetPrograms())
            {
                auto& program = level.GetProgramFromId(program_id);
                if (!program.SerializeEnable())
                    continue; // Skip internal programs such as DisplayProgram
                shader_names.insert(program.GetData().shader_vertex());
                shader_names.insert(program.GetData().shader_fragment());
            }
            for (const auto& shader : shader_names)
            {
                if (ImGui::MenuItem(shader.c_str()))
                {
                    menubar_view_.GetDrawGui().AddWindow(
                        std::make_unique<WindowGlslFile>(
                            std::string("asset/shader/opengl/") + shader,
                            device_,
                            menubar_file_.GetFileName()));
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
}

void Menubar::MenuView()
{
    if (ImGui::BeginMenu("View"))
    {
        if (ImGui::MenuItem("Fullscreen", "F11"))
        {
            auto& draw_gui = menubar_view_.GetDrawGui();
            auto is_visible = draw_gui.IsVisible();
            draw_gui.SetVisible(!is_visible);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Show Resolution", "Ctrl+R", &show_resolution_))
        {
            menubar_view_.ShowResolutionWindow();
        }
        if (ImGui::MenuItem("Show Log", "Ctrl+L", &show_logger_))
        {
            menubar_view_.ShowLoggerWindow();
        }
        ImGui::Separator();
        if (ImGui::BeginMenu("Texture"))
        {
            menubar_view_.ShowTexturesWindow(device_);
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
}

bool Menubar::DrawCallback()
{
    MenuFile();
    MenuEdit();
    MenuView();
    return true;
}

std::string Menubar::GetName() const
{
    return name_;
}

void Menubar::SetName(const std::string& name)
{
    name_ = name;
}

bool Menubar::End() const
{
    return end_;
}

void Menubar::SetFileName(
    const std::string& file_name, FileDialogEnum file_dialog_enum)
{
    switch (file_dialog_enum)
    {
    case FileDialogEnum::NEW:
        Logger::GetInstance()->info(
            std::format("New project file: {}", file_name));
        break;
    case FileDialogEnum::OPEN:
        Logger::GetInstance()->info(
            std::format("Open project file: {}", file_name));
        break;
    case FileDialogEnum::SAVE_AS:
        Logger::GetInstance()->info(
            std::format("Save project file as: {}", file_name));
        break;
    }
}

} // End namespace frame::gui.
