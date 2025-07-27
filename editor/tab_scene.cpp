#include "tab_scene.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

#include <unordered_set>

namespace frame::gui
{

void TabScene::BuildScene(LevelInterface& level)
{
    std::vector<EntityId> ids = level.GetSceneNodes();
    std::unordered_set<EntityId> alive(ids.begin(), ids.end());

    for (auto it = nodes_.begin(); it != nodes_.end();)
    {
        if (!alive.count(it->first))
        {
            it->second->destroy();
            it = nodes_.erase(it);
        }
        else
        {
            ++it;
        }
    }

    auto root = level.GetDefaultRootSceneNodeId();
    std::function<void(EntityId, int)> add = [&](EntityId id, int depth) {
        if (nodes_.count(id))
        {
            for (auto child : level.GetChildList(id))
                add(child, depth + 1);
            return;
        }
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
    if (root != frame::NullId)
        add(root, 0);

    for (const auto& [id, node] : nodes_)
    {
        auto in_pin = node->inPin("in");
        auto parent_id = level.GetParentId(id);
        if (parent_id != frame::NullId && nodes_.count(parent_id))
        {
            auto parent_pin = nodes_[parent_id]->outPin("out");
            auto link = in_pin->getLink();
            if (link.expired() || link.lock()->left() != parent_pin)
            {
                if (in_pin->isConnected())
                    in_pin->deleteLink();
                parent_pin->createLink(in_pin);
            }
        }
        else if (in_pin->isConnected())
        {
            in_pin->deleteLink();
        }
    }
}

void TabScene::Draw(LevelInterface& level)
{
    ImVec2 avail = ImGui::GetContentRegionAvail();
    node_flow_.setSize(avail);

    BuildScene(level);

    node_flow_.update();
}

} // namespace frame::gui
