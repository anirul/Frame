#include "window_level.h"

#include <imgui.h>
#include <fstream>
#include <cstdint>

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"

namespace frame::gui {

WindowLevel::WindowLevel(DeviceInterface& device, const std::string& file_name)
    : WindowJsonFile(file_name, device), device_(device) {}

bool WindowLevel::DrawCallback() {
    auto& level = device_.GetLevel();
    auto draw_toggle = [&]() {
        if (show_json_) {
            ImGui::BeginDisabled();
            ImGui::Button("JSON Editor");
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Node Editor"))
                show_json_ = false;
        } else {
            if (ImGui::Button("JSON Editor"))
                show_json_ = true;
            ImGui::SameLine();
            ImGui::BeginDisabled();
            ImGui::Button("Node Editor");
            ImGui::EndDisabled();
        }
    };

    draw_toggle();
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        // TODO: implement saving level
    }
    ImGui::Separator();

    if (show_json_) {
        WindowJsonFile::DrawCallback();
    } else {
        ImGui::Begin("Assets", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        tab_textures_.Draw(level);
        tab_programs_.Draw(level);
        tab_materials_.Draw(level);
        ImGui::End();

        tab_scene_.Draw(level);
    }
    return true;
}

} // namespace frame::gui
