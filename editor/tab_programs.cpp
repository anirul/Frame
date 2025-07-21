#include "tab_programs.h"
#include "frame/entity_id.h"
#include <algorithm>
#include <format>

#include <imgui.h>

namespace frame::gui {

void TabPrograms::Draw(LevelInterface& level) {
    if (current_program_) {
        if (ImGui::Button("Back")) {
            current_program_.reset();
            return;
        }
        ImGui::SameLine();
        ImGui::Text("%s", current_program_->GetName().c_str());
        current_program_->DrawCallback();
    } else if (ImGui::CollapsingHeader("Programs", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (auto id : level.GetPrograms()) {
            auto& prog = level.GetProgramFromId(id);
            if (ImGui::Selectable(prog.GetName().c_str())) {
                current_program_ = std::make_unique<WindowProgram>(level, prog);
                return; // Show program next frame
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
