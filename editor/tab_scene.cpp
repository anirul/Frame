#include "tab_scene.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <glm/glm.hpp>
#include <imgui.h>

#include "frame/node_matrix.h"

namespace frame::gui
{

void TabScene::BuildScene(LevelInterface& level)
{
    nodes_.clear();
    node_flow_ = ImFlow::ImNodeFlow();

    auto root = level.GetDefaultRootSceneNodeId();
    std::function<void(EntityId, int)> add = [&](EntityId id, int depth) {
        auto name = level.GetNameFromId(id);
        auto node = node_flow_.addLambdaNode(
            [](ImFlow::BaseNode*) {}, ImVec2(depth * 100.0f, depth * 60.0f));
        node->setTitle(name);
        node->addIN<int>("in", 0, ImFlow::ConnectionFilter::None());
        static_cast<void>(node->addOUT<int>("out"));
        nodes_[id] = node;
        for (auto child : level.GetChildList(id))
            add(child, depth + 1);
    };
    if (root)
        add(root, 0);

    for (const auto& [id, node] : nodes_)
    {
        auto parent_id = level.GetParentId(id);
        if (parent_id != frame::NullId && nodes_.count(parent_id))
        {
            nodes_[parent_id]->outPin("out")->createLink(node->inPin("in"));
        }
    }
}

void TabScene::Draw(LevelInterface& level)
{
    ImVec2 avail = ImGui::GetContentRegionAvail();
    node_flow_.setSize(avail);

    if (ImGui::Button("New Node"))
    {
        auto functor = [&level](const std::string& n) -> frame::NodeInterface* {
            auto maybe = level.GetIdFromName(n);
            if (!maybe)
                return nullptr;
            return &level.GetSceneNodeFromId(maybe);
        };
        auto root_id = level.GetDefaultRootSceneNodeId();
        std::string base = "Node";
        std::string name = base;
        int counter = 1;
        while (level.GetIdFromName(name) != frame::NullId)
            name = base + std::to_string(counter++);
        auto node =
            std::make_unique<frame::NodeMatrix>(functor, glm::mat4(1.0f));
        node->SetName(name);
        if (root_id != frame::NullId)
            node->SetParentName(level.GetNameFromId(root_id));
        level.AddSceneNode(std::move(node));
    }

    BuildScene(level);

    node_flow_.update();
}

} // namespace frame::gui
