#pragma once

#include "tab_interface.h"
#include "frame/gui/draw_gui_interface.h"
#include "frame/program_interface.h"
#include <imgui-node-editor/imgui_node_editor.h>

namespace frame::gui {

class TabPrograms : public TabInterface {
  public:
    explicit TabPrograms(DrawGuiInterface& draw_gui)
        : TabInterface("Programs"), draw_gui_(draw_gui) {}
    ~TabPrograms() override;
    void Draw(LevelInterface& level) override;

  private:
    DrawGuiInterface& draw_gui_;
    ax::NodeEditor::Config config_{};
    ax::NodeEditor::EditorContext* context_ = nullptr;
    frame::ProgramInterface* current_program_ = nullptr;
    bool initialized_ = false;
};

} // namespace frame::gui
