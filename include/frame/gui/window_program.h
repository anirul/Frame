#pragma once

#include <imgui-node-editor/imgui_node_editor.h>
#include <string>

#include "frame/gui/gui_window_interface.h"
#include "frame/level_interface.h"
#include "frame/program_interface.h"

namespace frame::gui
{

class WindowProgram : public GuiWindowInterface
{
  public:
    WindowProgram(LevelInterface& level, ProgramInterface& program);
    ~WindowProgram() override;

    bool DrawCallback() override;
    bool End() const override;
    std::string GetName() const override;
    void SetName(const std::string& name) override;

  private:
    LevelInterface& level_;
    ProgramInterface& program_;
    std::string editor_name_;
    ax::NodeEditor::Config config_{};
    ax::NodeEditor::EditorContext* context_ = nullptr;
    bool initialized_ = false;
    bool end_ = false;
    std::string name_;
};

} // namespace frame::gui
