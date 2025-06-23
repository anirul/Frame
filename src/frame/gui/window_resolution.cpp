#include "frame/gui/window_resolution.h"

#include <format>
#include <imgui.h>

#include <glm/gtc/type_ptr.hpp>

namespace frame::gui
{

WindowResolution::WindowResolution(
    const std::string& name,
    glm::uvec2 size,
    glm::uvec2 border_less_size,
    glm::vec2 pixel_per_inch,
    bool enable_stereo) :
    size_(size),
    border_less_size_(border_less_size),
    hppi_(pixel_per_inch.x),
    vppi_(pixel_per_inch.y),
    enable_stereo_(enable_stereo)
{
    for (int i = 0; i < resolutions_.size(); ++i)
    {
        if (resolutions_.at(i).values == size_)
        {
            resolution_selected_ = i;
        }
    }
    for (const auto& value : resolutions_)
    {
        resolution_items_.push_back(std::format(
            "{} - {}x{}", value.name, value.values.x, value.values.y));
    }
    fullscreen_items_.push_back("Windowed");
    fullscreen_items_.push_back("Full screen");
    fullscreen_items_.push_back("Window border less");
    stereo_items_.push_back("None");
    stereo_items_.push_back("Horizontal Split");
    stereo_items_.push_back("Horizontal Side-by-side");
    SetName(name);
}

bool WindowResolution::DrawCallback()
{
    ImGui::Text("Select resolution: ");
    if (ImGui::BeginCombo(
            "Designation", resolution_items_[resolution_selected_].c_str()))
    {
        for (int i = 0; i < resolution_items_.size(); ++i)
        {
            const bool isSelected = (resolution_selected_ == i);
            if (ImGui::Selectable(resolution_items_[i].c_str(), isSelected))
            {
                resolution_selected_ = i;
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::BeginCombo(
            "Full screen", fullscreen_items_[fullscreen_selected_].c_str()))
    {
        for (int i = 0; i < fullscreen_items_.size(); ++i)
        {
            const bool isSelected = (fullscreen_selected_ == i);
            if (ImGui::Selectable(fullscreen_items_[i].c_str(), isSelected))
            {
                fullscreen_selected_ = i;
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    if (enable_stereo_)
    {
        ImGui::Separator();
        if (ImGui::BeginCombo(
                "Stereo selection", stereo_items_[stereo_selected_].c_str()))
        {
            for (int i = 0; i < stereo_items_.size(); ++i)
            {
                const bool is_selected = (stereo_selected_ == i);
                if (ImGui::Selectable(stereo_items_[i].c_str(), is_selected))
                {
                    stereo_selected_ = i;
                }
                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::DragFloat(
            "Interocular distance", &interocular_distance_, 0.0f, 1.0f, 0.001f);
        ImGui::DragFloat3(
            "Focus point", glm::value_ptr(focus_point_), 0.0f, 1000.0f, 0.1f);
        ImGui::Checkbox("Invert Left and Right", &invert_left_right_);
    }
    ImGui::Separator();
    ImGui::Text("%s", std::format("Horizontal PPI: {}", hppi_).c_str());
    ImGui::Text("%s", std::format("Vertical PPI: {}", vppi_).c_str());
    ImGui::Separator();
    if (ImGui::Button("Change"))
    {
        size_ = resolutions_[resolution_selected_].values;
        fullscreen_ = fullscreen_mode_[fullscreen_selected_];
        stereo_ = stereo_mode_[stereo_selected_];
        end_ = false;
        return false;
    }
    end_ = true;
    return true;
}

std::string WindowResolution::GetName() const
{
    return name_;
}

void WindowResolution::SetName(const std::string& name)
{
    name_ = name;
}

bool WindowResolution::End() const
{
    return end_;
}

glm::uvec2 WindowResolution::GetSize() const
{
    if (fullscreen_ == FullScreenEnum::FULLSCREEN_DESKTOP)
    {
        return border_less_size_;
    }
    return size_;
}

} // End namespace frame::gui.
