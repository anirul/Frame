#include "tab_programs.h"
#include "frame/entity_id.h"
#include "frame/gui/window_message_box.h"
#include "window_new_program.h"
#include "frame/material_interface.h"
#include "frame/logger.h"

#include <imgui.h>

namespace frame::gui {

void TabPrograms::Draw(LevelInterface& level) {
    const float button_size = ImGui::GetFrameHeight();
    bool open = ImGui::CollapsingHeader(
        "Programs",
        ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap);
    ImVec2 header_min = ImGui::GetItemRectMin();
    ImVec2 header_max = ImGui::GetItemRectMax();
    ImGui::SetItemAllowOverlap();
    ImGui::SetCursorScreenPos({header_max.x - 2.f * button_size - 8.f, header_min.y});
    if (ImGui::Button("-##program", ImVec2(button_size, button_size))) {
        RemoveSelectedProgram(level);
    }
    ImGui::SetCursorScreenPos({header_max.x - button_size - 4.f, header_min.y});
    if (ImGui::Button("+##program", ImVec2(button_size, button_size))) {
        draw_gui_.AddModalWindow(std::make_unique<WindowNewProgram>(
            draw_gui_, level, update_json_callback_));
    }
    if (open) {
        for (auto id : level.GetPrograms()) {
            auto& prog = level.GetProgramFromId(id);
            bool selected = (id == selected_program_id_);
            if (ImGui::Selectable(prog.GetName().c_str(), selected)) {
                selected_program_id_ = id;
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

void TabPrograms::RemoveSelectedProgram(LevelInterface& level) {
    if (selected_program_id_ == frame::NullId) {
        draw_gui_.AddModalWindow(
            std::make_unique<WindowMessageBox>("Warning", "No program selected."));
        return;
    }
    if (IsProgramUsed(level, selected_program_id_)) {
        frame::Logger::GetInstance()->warn(
            "Cannot delete a program that is still used.");
        draw_gui_.AddModalWindow(std::make_unique<WindowMessageBox>(
            "Warning", "Cannot delete a program that is still used."));
        return;
    }
    level.RemoveProgram(selected_program_id_);
    selected_program_id_ = frame::NullId;
    if (update_json_callback_)
        update_json_callback_();
}

bool TabPrograms::IsProgramUsed(const LevelInterface& level, EntityId id) const {
    for (auto material_id : level.GetMaterials()) {
        const MaterialInterface& material = level.GetMaterialFromId(material_id);
        try {
            if (material.GetProgramId(&level) == id)
                return true;
        } catch (...) {
        }
    }
    return false;
}

} // namespace frame::gui
