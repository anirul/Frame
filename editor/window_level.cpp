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

    if (show_json_) {
        draw_toggle();
        ImGui::SameLine();
        WindowJsonFile::DrawCallback();
    } else {
        draw_toggle();
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            // TODO: implement saving level
        }
        ImGui::Separator();
        if (ImGui::BeginTabBar("##level_tabs")) {
            TabInterface* tabs[] = {&tab_textures_, &tab_programs_,
                                    &tab_materials_, &tab_scene_};
            for (TabInterface* tab : tabs) {
                if (ImGui::BeginTabItem(tab->GetName().c_str())) {
                    tab->Draw(level);
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
    }
    return true;
}

} // namespace frame::gui
