#include "tab_scene.h"

#include <cstdint>
#include <exception>
#include <functional>
#include <string>
#include <unordered_set>
#include <imgui.h>

namespace frame::gui
{

namespace
{

const char* NodeTypeToLabel(frame::NodeTypeEnum type)
{
    switch (type)
    {
    case frame::NodeTypeEnum::NODE_MATRIX:
        return "Matrix";
    case frame::NodeTypeEnum::NODE_MESH:
        return "Mesh";
    case frame::NodeTypeEnum::NODE_LIGHT:
        return "Light";
    case frame::NodeTypeEnum::NODE_CAMERA:
        return "Camera";
    case frame::NodeTypeEnum::NODE_UKNOWN:
    default:
        return "Unknown";
    }
}

} // namespace

void TabScene::Draw(LevelInterface& level)
{
    ImGui::TextUnformatted("Scene Tree");
    ImGui::Separator();

    const auto root_id = level.GetDefaultRootSceneNodeId();
    if (!root_id)
    {
        ImGui::TextUnformatted("No root scene node.");
        return;
    }

    std::unordered_set<EntityId> visited = {};
    std::function<void(EntityId)> draw_node = [&](EntityId node_id) {
        if (!node_id)
        {
            return;
        }
        if (visited.contains(node_id))
        {
            ImGui::Text(
                "Cyclic reference detected (id=%lld).",
                static_cast<long long>(node_id));
            return;
        }
        visited.insert(node_id);

        std::string node_name = "<invalid>";
        const frame::NodeInterface* node_ptr = nullptr;
        try
        {
            node_name = level.GetNameFromId(node_id);
            node_ptr = &level.GetSceneNodeFromId(node_id);
        }
        catch (const std::exception&)
        {
            ImGui::Text(
                "Invalid scene node (id=%lld).",
                static_cast<long long>(node_id));
            return;
        }

        const auto children = level.GetChildList(node_id);
        const bool is_leaf = children.empty();
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
        if (is_leaf)
        {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }
        else
        {
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }

        const char* node_type = NodeTypeToLabel(node_ptr->GetNodeType());
        const bool open = ImGui::TreeNodeEx(
            reinterpret_cast<void*>(static_cast<std::uintptr_t>(node_id)),
            flags,
            "%s [%s]##%lld",
            node_name.c_str(),
            node_type,
            static_cast<long long>(node_id));
        if (!is_leaf && open)
        {
            for (const auto child_id : children)
            {
                draw_node(child_id);
            }
            ImGui::TreePop();
        }
    };

    draw_node(root_id);

}

} // namespace frame::gui
