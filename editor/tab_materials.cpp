#include "tab_materials.h"

#include <imgui.h>

namespace frame::gui {

void TabMaterials::Draw(LevelInterface& level) {
    for (auto id : level.GetMaterials()) {
        auto& mat = level.GetMaterialFromId(id);
        ImGui::BulletText("%s", mat.GetName().c_str());
    }
}

} // namespace frame::gui
