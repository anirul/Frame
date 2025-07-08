#include "tab_programs.h"

#include <imgui.h>

namespace frame::gui {

void TabPrograms::Draw(LevelInterface& level) {
    for (auto id : level.GetPrograms()) {
        auto& prog = level.GetProgramFromId(id);
        ImGui::BulletText("%s", prog.GetName().c_str());
    }
}

} // namespace frame::gui
