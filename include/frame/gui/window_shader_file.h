#pragma once

#include <string>
#include "TextEditor.h"

#include "frame/gui/gui_window_interface.h"
#include "frame/file/file_system.h"

namespace frame::gui {

class WindowShaderFile : public GuiWindowInterface {
  public:
    explicit WindowShaderFile(const std::string& file_name);
    ~WindowShaderFile() override = default;

    bool DrawCallback() override;
    bool End() const override;
    std::string GetName() const override;
    void SetName(const std::string& name) override;

  private:
    std::string name_;
    std::string file_name_;
    TextEditor editor_;
    bool end_ = false;
    std::string error_message_;
};

} // End namespace frame::gui.
