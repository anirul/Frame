#pragma once

#include "frame/gui/gui_window_interface.h"
#include "menubar_file.h"

namespace frame::gui
{

class WindowStart : public GuiWindowInterface
{
  public:
    explicit WindowStart(MenubarFile& menubar_file);
    ~WindowStart() override = default;

    bool DrawCallback() override;
    bool End() const override;
    std::string GetName() const override;
    void SetName(const std::string& name) override;

  private:
    MenubarFile& menubar_file_;
    std::string name_ = "Start";
    bool end_ = false;
};

} // namespace frame::gui
