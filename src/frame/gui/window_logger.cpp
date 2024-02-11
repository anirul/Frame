#include "frame/gui/window_logger.h"

#include <imgui.h>

namespace frame::gui
{

WindowLogger::WindowLogger(const std::string& name)
    : name_(name)
{
    SetName(name);
}

bool WindowLogger::DrawCallback()
{
    ImGui::BeginChild("Scrolling");
    for (const auto& message : logger_.GetLastLogs(100))
    {
        ImGui::TextUnformatted(message.c_str());
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
    return true;
}

std::string WindowLogger::GetName() const
{
    return name_;
}

void WindowLogger::SetName(const std::string& name)
{
    name_ = name;
}

bool WindowLogger::End() const
{
    return false;
}

} // namespace frame::gui.
