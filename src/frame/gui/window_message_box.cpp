#include "frame/gui/window_message_box.h"

#include <imgui.h>

namespace frame::gui
{

WindowMessageBox::WindowMessageBox(
    const std::string& name, const std::string& message)
    : name_(name), message_(message)
{
}

bool WindowMessageBox::DrawCallback()
{
    ImGui::TextWrapped("%s", message_.c_str());
    if (ImGui::Button("Ok"))
    {
        end_ = true;
    }
    return true;
}

bool WindowMessageBox::End() const
{
    return end_;
}

std::string WindowMessageBox::GetName() const
{
    return name_;
}

void WindowMessageBox::SetName(const std::string& name)
{
    name_ = name;
}

} // namespace frame::gui
