#include "frame/gui/window_logger.h"

#include <imgui.h>

namespace frame::gui
{

WindowLogger::WindowLogger(const std::string& name) : name_(name)
{
    SetName(name);
}

bool WindowLogger::DrawCallback()
{
    ImGui::BeginChild("Scrolling");
    for (const auto& log_message : logger_.GetLastLogs(100))
    {
        LogWithColor(log_message);
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

void WindowLogger::LogWithColor(const LogMessage& log_message) const
{
    switch (log_message.level)
    {
    case spdlog::level::trace:
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        break;
    case spdlog::level::debug:
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
        break;
    case spdlog::level::info:
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        break;
    case spdlog::level::warn:
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.5f, 1.0f));
        break;
    case spdlog::level::err:
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
        break;
    case spdlog::level::critical:
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        break;
    case spdlog::level::off:
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        break;
    default:
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        break;
    }
    ImGui::TextUnformatted(log_message.message.c_str());
    ImGui::PopStyleColor();
}

} // namespace frame::gui.
