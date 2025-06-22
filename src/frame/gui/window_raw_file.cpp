#include "frame/gui/window_raw_file.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <imgui.h>

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"

namespace frame::gui
{

WindowRawFile::WindowRawFile(
    const std::string& file_name, DeviceInterface& device)
    : name_("Raw Level Edit"), file_name_(file_name), device_(device)
{
    buffer_.resize(64 * 1024, '\0');
    try
    {
        std::ifstream file(frame::file::FindFile(file_name_));
        if (file)
        {
            std::string content(
                (std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>());
            std::size_t len = std::min(content.size(), buffer_.size() - 1);
            std::memcpy(buffer_.data(), content.data(), len);
            buffer_[len] = '\0';
        }
    }
    catch (...)
    {
        // If file not found just keep empty buffer
    }
}

bool WindowRawFile::DrawCallback()
{
    if (ImGui::Button("Reload"))
    {
        std::string content(buffer_.data());
        auto level = frame::json::ParseLevel(device_.GetSize(), content);
        device_.Startup(std::move(level));
    }
    ImGui::SameLine();
    if (ImGui::Button("Close"))
    {
        end_ = true;
    }
    ImGui::Separator();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::InputTextMultiline(
        "##rawtext",
        buffer_.data(),
        buffer_.size(),
        avail,
        ImGuiInputTextFlags_AllowTabInput);
    return true;
}

bool WindowRawFile::End() const
{
    return end_;
}

std::string WindowRawFile::GetName() const
{
    return name_;
}

void WindowRawFile::SetName(const std::string& name)
{
    name_ = name;
}

} // End namespace frame::gui.
