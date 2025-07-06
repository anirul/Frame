#pragma once

#include "frame/device_interface.h"
#include "frame/gui/gui_window_interface.h"

namespace frame::gui
{

class WindowLevel : public GuiWindowInterface
{
  public:
    explicit WindowLevel(DeviceInterface& device);
    ~WindowLevel() override = default;

    bool DrawCallback() override;
    bool End() const override;
    std::string GetName() const override;
    void SetName(const std::string& name) override;

  private:
    void DisplayNode(LevelInterface& level, EntityId id);

    DeviceInterface& device_;
    std::string name_ = "Level Editor";
    bool end_ = false;
};

} // namespace frame::gui
