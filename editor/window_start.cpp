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
    constexpr float side_space = 20.0f;
    ImGui::Dummy(ImVec2(side_space, 0.0f));
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    ImGui::Text("Welcome to Frame Editor");
    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    if (ImGui::Button("Create New Project"))
    {
        ImGui::CloseCurrentPopup();
        menubar_file_.ShowNewProject();
        end_ = true;
    }
    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    if (ImGui::Button("Open Existing Project"))
    {
        ImGui::CloseCurrentPopup();
        menubar_file_.ShowOpenProject();
        end_ = true;
    }
    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(side_space, 0.0f));
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
