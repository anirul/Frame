#include "window_start.h"

#include <imgui.h>

namespace frame::gui
{

WindowStart::WindowStart(MenubarFile& menubar_file)
    : menubar_file_(menubar_file)
{
}

bool WindowStart::DrawCallback()
{
    ImGui::Text("Welcome to Frame Editor");
    if (ImGui::Button("Create New Project"))
    {
        ImGui::CloseCurrentPopup();
        menubar_file_.ShowNewProject();
        end_ = true;
    }
    if (ImGui::Button("Open Existing Project"))
    {
        ImGui::CloseCurrentPopup();
        menubar_file_.ShowOpenProject();
        end_ = true;
    }
    return true;
}

bool WindowStart::End() const
{
    return end_;
}

std::string WindowStart::GetName() const
{
    return name_;
}

void WindowStart::SetName(const std::string& name)
{
    name_ = name;
}

} // namespace frame::gui
