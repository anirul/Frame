#include "tab_textures.h"
#include "frame/entity_id.h"

#include <imgui.h>

namespace frame::gui {

void TabTextures::Draw(LevelInterface& level) {
    if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (auto id : level.GetTextures()) {
            auto& tex = level.GetTextureFromId(id);
            ImGui::Selectable(tex.GetName().c_str());
            if (ImGui::BeginDragDropSource()) {
                EntityId payload = id;
                ImGui::SetDragDropPayload("FRAME_ASSET_ID", &payload, sizeof(payload));
                ImGui::Text("%s", tex.GetName().c_str());
                ImGui::EndDragDropSource();
            }
        }
    }
}

} // namespace frame::gui
