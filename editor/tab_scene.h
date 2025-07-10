#pragma once

#include "tab_interface.h"
#include "frame/entity_id.h"
#include <imgui-node-editor/imgui_node_editor.h>

namespace frame::gui {

class TabScene : public TabInterface {
  public:
    TabScene() : TabInterface("Scene") {}
    ~TabScene() override;

    void Draw(LevelInterface& level) override;

  private:
    void DisplayNode(LevelInterface& level, EntityId id, EntityId parent);
    void ProcessEvents(LevelInterface& level);

    int next_node_index_ = 0;

    ax::NodeEditor::EditorContext* context_ = nullptr;
};

} // namespace frame::gui
