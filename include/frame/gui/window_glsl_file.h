#pragma once

#include "TextEditor.h"
#include <string>

#include "frame/device_interface.h"
#include "frame/file/file_system.h"
#include "frame/gui/gui_window_interface.h"
#include "frame/opengl/shader.h"

namespace frame::gui
{

class WindowGlslFile : public GuiWindowInterface
{
  public:
    WindowGlslFile(const std::string& file_name, DeviceInterface& device);
    ~WindowGlslFile() override = default;

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
