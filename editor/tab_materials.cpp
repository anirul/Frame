#include "tab_materials.h"
#include "frame/entity_id.h"

#include <imgui.h>

namespace frame::gui {

void TabMaterials::Draw(LevelInterface& level) {
    if (ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (auto id : level.GetMaterials()) {
            auto& mat = level.GetMaterialFromId(id);
            ImGui::Selectable(mat.GetName().c_str());
            if (ImGui::BeginDragDropSource()) {
                EntityId payload = id;
                ImGui::SetDragDropPayload("FRAME_ASSET_ID", &payload, sizeof(payload));
                ImGui::Text("%s", mat.GetName().c_str());
                ImGui::EndDragDropSource();
            }
        }
    }
}

} // namespace frame::gui
