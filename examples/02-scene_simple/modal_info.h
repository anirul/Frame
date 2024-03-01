#pragma once

#include "frame/gui/gui_window_interface.h"
#include <imgui.h>

class ModalInfo : public frame::gui::GuiWindowInterface
{
  public:
    ModalInfo(const std::string& name, const std::string& text) :
		name_(name), text_(text)
    {
    }
    bool DrawCallback() override
    {
        ImGui::TextUnformatted(text_.c_str());
        if (ImGui::Button("Ok"))
        {
            end_ = true;
        }
        return true;
    }
    bool End() const override
    {
        return end_;
    }
    std::string GetName() const override
    {
        return name_;
    }
    void SetName(const std::string& name) override
    {
        name_ = name;
    }

  private:
    bool end_ = false;
    std::string name_;
	std::string text_;
};
