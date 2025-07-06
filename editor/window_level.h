#pragma once

#include "frame/device_interface.h"
#include "frame/gui/window_json_file.h"
#include <imgui-node-editor/imgui_node_editor.h>

namespace frame::gui
{

class WindowLevel : public WindowJsonFile
{
  public:
    WindowLevel(DeviceInterface& device, const std::string& file_name);
    ~WindowLevel() override;

    bool DrawCallback() override;

  private:
    void DisplayNode(LevelInterface& level, EntityId id, EntityId parent);

    DeviceInterface& device_;
    ax::NodeEditor::EditorContext* context_ = nullptr;
    bool show_json_ = false;
};

} // namespace frame::gui
