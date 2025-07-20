#include "frame/gui/window_message_box.h"

#include <imgui.h>

namespace frame::gui {

WindowMessageBox::WindowMessageBox(const std::string& name, const std::string& message)
    : name_(name), message_(message) {}

bool WindowMessageBox::DrawCallback() {
    // Ensure the modal has a reasonable width so text doesn't wrap too narrowly
    // and remains readable. We do this here rather than through window flags
    // because the popup is created by the draw loop with auto resize enabled.
    ImGui::SetWindowSize(ImVec2(400.0f, 0.0f));
    ImGui::TextWrapped("%s", message_.c_str());
    if (ImGui::Button("Ok")) {
        end_ = true;
    }
    return true;
}

bool WindowMessageBox::End() const { return end_; }

std::string WindowMessageBox::GetName() const { return name_; }

void WindowMessageBox::SetName(const std::string& name) { name_ = name; }

} // namespace frame::gui
