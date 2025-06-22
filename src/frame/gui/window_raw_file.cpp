#include "frame/gui/window_raw_file.h"

#include <cstring>
#include <fstream>
#include <imgui.h>

#include "frame/file/file_system.h"

namespace frame::gui
{

WindowRawFile::WindowRawFile(const std::string& file_name)
    : name_("Raw Level Edit"), file_name_(file_name)
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
    ImGui::InputTextMultiline(
        "##rawtext",
        buffer_.data(),
        buffer_.size(),
        ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 30),
        ImGuiInputTextFlags_AllowTabInput);
    if (ImGui::Button("Save"))
    {
        std::ofstream file(file_name_);
        if (file)
        {
            file.write(buffer_.data(), std::strlen(buffer_.data()));
        }
        end_ = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
    {
        end_ = true;
    }
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
