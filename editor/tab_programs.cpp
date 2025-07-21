#include "tab_programs.h"
#include "frame/entity_id.h"
#include <algorithm>
#include <format>

#include <imgui.h>

namespace frame::gui {

void TabPrograms::Draw(LevelInterface& level) {
    if (ImGui::CollapsingHeader("Programs", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto open_windows = draw_gui_.GetWindowTitles();
        for (auto id : level.GetPrograms()) {
            auto& prog = level.GetProgramFromId(id);
            bool selected = ImGui::Selectable(prog.GetName().c_str());
            if (selected) {
                std::string window_name = std::format("program - [{}]", prog.GetName());
                if (std::find(open_windows.begin(), open_windows.end(), window_name) == open_windows.end()) {
                    draw_gui_.AddWindow(std::make_unique<WindowProgram>(level, prog));
                }
            }
            if (ImGui::BeginDragDropSource()) {
                EntityId payload = id;
                ImGui::SetDragDropPayload("FRAME_ASSET_ID", &payload, sizeof(payload));
                ImGui::Text("%s", prog.GetName().c_str());
                ImGui::EndDragDropSource();
            }
        }
    }
}

} // namespace frame::gui
