#include "tab_scene.h"

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
    ed::End();
    ed::SetCurrentEditor(nullptr);
}

} // namespace frame::gui
