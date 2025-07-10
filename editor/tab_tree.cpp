#include "tab_tree.h"
#include "frame/entity_id.h"

#include <imgui.h>

namespace frame::gui {

void TabTree::DrawNode(LevelInterface& level, EntityId id) {
    auto name = level.GetNameFromId(id);
    if (ImGui::TreeNode(name.c_str())) {
        for (auto child : level.GetChildList(id)) {
            DrawNode(level, child);
        }
        ImGui::TreePop();
    }
}

void TabTree::Draw(LevelInterface& level) {
    auto root = level.GetDefaultRootSceneNodeId();
    if (root)
        DrawNode(level, root);
}

} // namespace frame::gui
