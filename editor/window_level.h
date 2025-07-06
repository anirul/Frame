#pragma once

#include "frame/device_interface.h"
#include "frame/gui/gui_window_interface.h"
#include <imgui-node-editor/imgui_node_editor.h>

namespace frame::gui
{

class WindowLevel : public GuiWindowInterface
{
  public:
    explicit WindowLevel(DeviceInterface& device);
    ~WindowLevel() override;

    bool DrawCallback() override;
    bool End() const override;
    std::string GetName() const override;
    void SetName(const std::string& name) override;

  private:
    void DisplayNode(LevelInterface& level, EntityId id);

    DeviceInterface& device_;
    ax::NodeEditor::EditorContext* context_ = nullptr;
    std::string name_ = "Level Editor";
    bool end_ = false;
};

} // namespace frame::gui
