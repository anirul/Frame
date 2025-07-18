#include "tab_scene.h"

#include "frame/node_matrix.h"

#include <imgui.h>

namespace ed = ax::NodeEditor;

namespace frame::gui {

TabScene::~TabScene() {
    if (context_)
        ed::DestroyEditor(context_);
}

void TabScene::DisplayNode(LevelInterface& level, EntityId id, EntityId parent) {
    auto name = level.GetNameFromId(id);
    ed::BeginNode(id);

    auto input_id = id * 2 + 1;
    auto output_id = id * 2 + 2;

    ed::BeginPin(input_id, ed::PinKind::Input);
    ImGui::Dummy(ImVec2(10, 10));
    ed::EndPin();

    ImGui::SameLine();
    ImGui::Text("%s", name.c_str());
    ImGui::SameLine();

    ed::BeginPin(output_id, ed::PinKind::Output);
    ImGui::Dummy(ImVec2(10, 10));
    ed::EndPin();

    ed::EndNode();

    if (parent != frame::NullId) {
        auto parent_output = parent * 2 + 2;
        std::uint64_t link_id = (static_cast<std::uint64_t>(parent) << 32) |
                                static_cast<std::uint64_t>(id);
        ed::Link(static_cast<ed::LinkId>(link_id), parent_output, input_id);
    }

    for (auto child : level.GetChildList(id)) {
        DisplayNode(level, child, id);
    }
}

void TabScene::Draw(LevelInterface& level) {
    if (!context_)
        context_ = ed::CreateEditor();

    ed::SetCurrentEditor(context_);
    ed::Begin("SceneEditor");
    auto root = level.GetDefaultRootSceneNodeId();
    if (root)
        DisplayNode(level, root, frame::NullId);
    ProcessEvents(level);
    ed::End();
    ed::SetCurrentEditor(nullptr);
}

void TabScene::ProcessEvents(LevelInterface& level) {
    ed::BeginCreate();
    ed::PinId start_pin, end_pin;
    if (ed::QueryNewLink(&start_pin, &end_pin)) {
        bool start_is_input = (static_cast<int>(start_pin.Get()) % 2) == 1;
        bool end_is_input = (static_cast<int>(end_pin.Get()) % 2) == 1;
        ed::PinId input_id = start_is_input ? start_pin : end_pin;
        ed::PinId output_id = start_is_input ? end_pin : start_pin;
        if (start_is_input == end_is_input)
            ed::RejectNewItem();
        else if (ed::AcceptNewItem()) {
            EntityId child = (static_cast<int>(input_id.Get()) - 1) / 2;
            EntityId parent = (static_cast<int>(output_id.Get()) - 2) / 2;
            auto parent_name = level.GetNameFromId(parent);
            level.GetSceneNodeFromId(child).SetParentName(parent_name);
            std::uint64_t link_id =
                (static_cast<std::uint64_t>(parent) << 32) |
                static_cast<std::uint64_t>(child);
            ed::Link(static_cast<ed::LinkId>(link_id), output_id, input_id);
        }
    }
    ed::EndCreate();

    ed::BeginDelete();
    ed::LinkId link_id;
    while (ed::QueryDeletedLink(&link_id)) {
        if (ed::AcceptDeletedItem()) {
            ed::DeleteLink(link_id);
            std::uint64_t lid = link_id.Get();
            EntityId child = static_cast<EntityId>(lid & 0xffffffffu);
            level.GetSceneNodeFromId(child).SetParentName("");
        }
    }
    ed::NodeId node_id;
    while (ed::QueryDeletedNode(&node_id)) {
        if (ed::AcceptDeletedItem()) {
            ed::DeleteNode(node_id);
            EntityId id = static_cast<EntityId>(node_id.Get());
            level.RemoveSceneNode(id);
        }
    }
    ed::EndDelete();

    if (ImGui::BeginDragDropTarget()) {
        if (auto* payload = ImGui::AcceptDragDropPayload("FRAME_ASSET_ID")) {
            EntityId id = *static_cast<const EntityId*>(payload->Data);
            auto functor = [&level](const std::string& name) -> frame::NodeInterface* {
                auto maybe = level.GetIdFromName(name);
                if (!maybe)
                    return nullptr;
                return &level.GetSceneNodeFromId(maybe);
            };
            auto node = std::make_unique<frame::NodeMatrix>(functor, glm::mat4(1.0f));
            node->GetData().set_name(std::string("node_") + std::to_string(++next_node_index_));
            level.AddSceneNode(std::move(node));
        }
        ImGui::EndDragDropTarget();
    }
}

} // namespace frame::gui
