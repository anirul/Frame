#include "tab_scene.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

namespace frame::gui
{

void TabScene::Draw(LevelInterface& level)
{
    ImVec2 avail = ImGui::GetContentRegionAvail();
    node_flow_.setSize(avail);

    if (!initialized_)
    {
        auto root = level.GetDefaultRootSceneNodeId();
        std::function<void(EntityId, int)> add = [&](EntityId id, int depth) {
            auto name = level.GetNameFromId(id);
            node_flow_.addLambdaNode(
                [name](ImFlow::BaseNode*) { ImGui::Text("%s", name.c_str()); },
                ImVec2(depth * 50.0f, depth * 20.0f));
            for (auto child : level.GetChildList(id))
                add(child, depth + 1);
        };
        if (root)
            add(root, 0);
        initialized_ = true;
    }
    node_flow_.update();
}

} // namespace frame::gui
