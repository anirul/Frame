#pragma once

#include "TextEditor.h"
#include "frame/device_interface.h"
#include "frame/gui/gui_window_interface.h"
#include <imgui-node-editor/imgui_node_editor.h>

namespace frame::gui
{

class WindowLevel : public GuiWindowInterface
{
  public:
    WindowLevel(DeviceInterface& device, const std::string& file_name);
    ~WindowLevel() override;

    bool DrawCallback() override;
    bool End() const override;
    std::string GetName() const override;
    void SetName(const std::string& name) override;

  private:
    void DisplayNode(LevelInterface& level, EntityId id, EntityId parent);

    DeviceInterface& device_;
    ax::NodeEditor::EditorContext* context_ = nullptr;
    std::string name_ = "Level Editor";
    bool end_ = false;
    bool show_json_ = false;
    std::string file_name_;
    TextEditor editor_;
    std::string error_message_;
};

} // namespace frame::gui
