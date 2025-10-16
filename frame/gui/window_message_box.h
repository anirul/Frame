#pragma once

#include "frame/gui/gui_window_interface.h"
#include <glm/glm.hpp>
#include <string>

namespace frame::gui
{

class WindowMessageBox : public GuiWindowInterface
{
  public:
    WindowMessageBox(const std::string& name, const std::string& message);
    ~WindowMessageBox() override = default;

    bool DrawCallback() override;
    bool End() const override;
    std::string GetName() const override;
    void SetName(const std::string& name) override;
    glm::vec2 GetInitialSize() const override
    {
        return glm::vec2(250.f, 0.f);
    }

  private:
    bool end_ = false;
    std::string name_;
    std::string message_;
};

} // namespace frame::gui
