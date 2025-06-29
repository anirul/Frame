#pragma once

#include <string>
#include <vector>
#include "TextEditor.h"

#include "frame/gui/gui_window_interface.h"
#include "frame/device_interface.h"

namespace frame::gui
{

/**
 * @class WindowJsonFile
 * @brief Simple window to edit a file as JSON text.
 */
class WindowJsonFile : public GuiWindowInterface
{
  public:
    WindowJsonFile(const std::string& file_name, DeviceInterface& device);
    ~WindowJsonFile() override = default;

    bool DrawCallback() override;
    bool End() const override;
    std::string GetName() const override;
    void SetName(const std::string& name) override;

  private:
    std::string name_;
    std::string file_name_;
    DeviceInterface& device_;
    TextEditor editor_;
    bool end_ = false;
    std::string error_message_;
};

} // End namespace frame::gui.
