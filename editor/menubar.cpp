#include "menubar.h"

#include <imgui.h>
#include "frame/gui/window_logger.h"
#include "frame/gui/window_resolution.h"

namespace frame::gui
{

Menubar::Menubar(
    const std::string& name,
    ViewWindows& view_windows)
    : name_(name), view_windows_(view_windows)
{
    SetName(name);
}

void Menubar::MenuFile()
{
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("New Project", "Ctrl+N"))
        {
        }
        if (ImGui::MenuItem("Open Project", "Ctrl+O"))
        {
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Save Project", "Ctrl+S"))
        {
        }
        if (ImGui::MenuItem("Save Project As...", "Ctrl+Shift+S"))
        {
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
        ImGui::EndMenu();
    }
}

void Menubar::MenuView()
{
    if (ImGui::BeginMenu("View"))
    {
        if (ImGui::MenuItem("Fullscreen", "F11"))
        {
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Show Resolution", "Ctrl+R", &show_resolution_))
        {
            if (show_resolution_)
            {
                view_windows_.CreateResolution("Resolution");
            }
            else
            {
                view_windows_.DeleteResolution("Resolution");
            }
        }
        if (ImGui::MenuItem("Show Log", "Ctrl+L", &show_logger_))
        {
            if (show_logger_)
            {
                view_windows_.CreateLogger("Logger");
            }
            else
            {
                view_windows_.DeleteLogger("Logger");
            }
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

} // End namespace frame::gui.
