#include "frame/gui/window_json_file.h"

#include "frame/logger.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <imgui.h>

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"

namespace frame::gui
{

WindowJsonFile::WindowJsonFile(
    const std::string& file_name, DeviceInterface& device)
    : name_("JSON Level Edit"), file_name_(file_name), device_(device)
{
    editor_.SetLanguageDefinition(TextEditor::LanguageDefinitionId::Json);
    try
    {
        std::ifstream file(frame::file::FindFile(file_name_));
        if (file)
        {
            std::string content(
                (std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>());
            editor_.SetText(content);
        }
    }
    catch (...)
    {
        // If file not found just keep empty buffer
    }
}

bool WindowJsonFile::DrawCallback()
{
    if (ImGui::Button("Build"))
    {
        try
        {
            std::string content = editor_.GetText();
            auto level = frame::json::ParseLevel(device_.GetSize(), content);
            device_.Startup(std::move(level));
            error_message_.clear();
        }
        catch (const std::exception& e)
        {
            error_message_ = e.what();
            frame::Logger::GetInstance()->error(e.what());
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload"))
    {
        try
        {
            std::ifstream file(frame::file::FindFile(file_name_));
            if (file)
            {
                std::string content(
                    (std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());
                editor_.SetText(content);
            }
            error_message_.clear();
        }
        catch (const std::exception& e)
        {
            error_message_ = e.what();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save"))
    {
        try
        {
            std::ofstream out(frame::file::FindFile(file_name_));
            out << editor_.GetText();
            error_message_.clear();
        }
        catch (const std::exception& e)
        {
            error_message_ = e.what();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Close"))
    {
        end_ = true;
    }
    ImGui::Separator();
    if (!error_message_.empty())
    {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float text_height =
            ImGui::CalcTextSize(error_message_.c_str(), nullptr, false, avail.x)
                .y;
        float padding = ImGui::GetStyle().WindowPadding.y;
        text_height = std::ceil(text_height + padding * 2);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
        ImGui::BeginChild(
            "##error_message",
            ImVec2(0, text_height),
            true,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::TextWrapped("%s", error_message_.c_str());
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Separator();
    }
    ImVec2 avail = ImGui::GetContentRegionAvail();
    bool focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    if (focused)
    {
        editor_.Render("##jsontext", true, avail, false);
    }
    else
    {
        ImGui::BeginDisabled();
        editor_.Render("##jsontext", true, avail, false);
        ImGui::EndDisabled();
    }
    return true;
}

bool WindowJsonFile::End() const
{
    return end_;
}

std::string WindowJsonFile::GetName() const
{
    return name_;
}

void WindowJsonFile::SetName(const std::string& name)
{
    name_ = name;
}

} // End namespace frame::gui.
