#include "frame/gui/window_resolution.h"

#include <fmt/core.h>
#include <imgui.h>

namespace frame::gui {

bool WindowResolution::DrawCallback() {
    ImGui::Text("Select resolution: ");
    if (ImGui::BeginCombo("Designation", resolution_items_[resolution_selected_].c_str())) {
        for (int i = 0; i < resolution_items_.size(); ++i) {
            const bool isSelected = (resolution_selected_ == i);
            if (ImGui::Selectable(resolution_items_[i].c_str(), isSelected)) {
                resolution_selected_ = i;
            }

            // Set the initial focus when opening the combo
            // (scrolling + keyboard navigation focus)
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::BeginCombo("Full screen", fullscreen_items_[fullscreen_selected_].c_str())) {
        for (int i = 0; i < fullscreen_items_.size(); ++i) {
            const bool isSelected = (fullscreen_selected_ == i);
            if (ImGui::Selectable(fullscreen_items_[i].c_str(), isSelected)) {
                fullscreen_selected_ = i;
            }

            // Set the initial focus when opening the combo
            // (scrolling + keyboard navigation focus)
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::Button("Change")) {
        size_       = resolutions_[resolution_selected_].values;
        fullscreen_ = fullscreen_mode_[fullscreen_selected_];
        end_        = false;
        return false;
    }
    end_ = true;
    return true;
}

std::string WindowResolution::GetName() const { return name_; }

void WindowResolution::SetName(const std::string& name) { name_ = name; }

bool WindowResolution::End() { return end_; }

WindowResolution::WindowResolution(const std::string& name,
                                   std::pair<std::uint32_t, std::uint32_t> size,
                                   std::pair<std::uint32_t, std::uint32_t> border_less_size)
    : size_(size), border_less_size_(border_less_size) {
    for (int i = 0; i < resolutions_.size(); ++i) {
        if (resolutions_.at(i).values == size_) {
            resolution_selected_ = i;
        }
    }
    for (const auto& value : resolutions_) {
        resolution_items_.push_back(
            fmt::format("{} - {}x{}", value.name, value.values.first, value.values.second));
    }
    fullscreen_items_.push_back("Windowed");
    fullscreen_items_.push_back("Full screen");
    fullscreen_items_.push_back("Window border less");
    SetName(name);
}

std::pair<std::uint32_t, std::uint32_t> WindowResolution::GetSize() const {
    if (fullscreen_ == FullScreenEnum::FULLSCREEN_DESKTOP) {
        return border_less_size_;
    }
    return size_;
}

}  // End namespace frame::gui.
