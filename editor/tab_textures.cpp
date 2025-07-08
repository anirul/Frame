#include "tab_textures.h"

#include <imgui.h>

namespace frame::gui {

void TabTextures::Draw(LevelInterface& level) {
    for (auto id : level.GetTextures()) {
        auto& tex = level.GetTextureFromId(id);
        ImGui::BulletText("%s", tex.GetName().c_str());
    }
}

} // namespace frame::gui
