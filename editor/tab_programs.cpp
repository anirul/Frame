#include "tab_programs.h"
#include "frame/entity_id.h"
#include "frame/gui/window_program.h"
#include <format>
#include <imgui.h>

namespace frame::gui {

void TabPrograms::Draw(LevelInterface& level)
{
    if (ImGui::CollapsingHeader("Programs", ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (auto id : level.GetPrograms())
        {
            auto& prog = level.GetProgramFromId(id);
            if (ImGui::Selectable(prog.GetName().c_str()))
            {
                std::string window_name = std::format("program - [{}]", prog.GetName());
                bool exists = false;
                for (const std::string& title : draw_gui_.GetWindowTitles())
                {
                    if (title == window_name)
                    {
                        exists = true;
                        break;
                    }
                }
                if (!exists)
                {
                    draw_gui_.AddWindow(std::make_unique<WindowProgram>(level, prog));
                }
            }
            if (ImGui::BeginDragDropSource())
            {
                EntityId payload = id;
                ImGui::SetDragDropPayload("FRAME_ASSET_ID", &payload, sizeof(payload));
                ImGui::Text("%s", prog.GetName().c_str());
                ImGui::EndDragDropSource();
            }
        }
    }
}

} // namespace frame::gui
