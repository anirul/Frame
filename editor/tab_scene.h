#pragma once

#include "tab_interface.h"
#include "frame/entity_id.h"
#include <imgui-node-editor/imgui_node_editor.h>

namespace frame::gui {

class TabScene : public TabInterface {
  public:
    TabScene() = default;
    ~TabScene() override;

    std::string GetName() const override { return "Scene"; }
    void Draw(LevelInterface& level) override;

  private:
    void DisplayNode(LevelInterface& level, EntityId id, EntityId parent);

    ax::NodeEditor::EditorContext* context_ = nullptr;
};

} // namespace frame::gui
