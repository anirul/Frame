#include "frame/gui/window_new_program.h"

#include <fstream>
#include <imgui.h>
#include <format>

#include "frame/file/file_system.h"
#include "frame/opengl/file/load_program.h"
#include "frame/opengl/program.h"
#include "frame/logger.h"
#include "frame/gui/window_message_box.h"

namespace frame::gui {

WindowNewProgram::WindowNewProgram(
    DrawGuiInterface& draw_gui,
    LevelInterface& level,
    std::function<void()> update_json_callback)
    : draw_gui_(draw_gui), level_(level),
      update_json_callback_(std::move(update_json_callback)) {}

bool WindowNewProgram::DrawCallback() {
    ImGui::InputText("Name", name_buffer_, sizeof(name_buffer_));
    ImGui::InputText("Vertex Shader", vertex_buffer_, sizeof(vertex_buffer_));
    ImGui::InputText("Fragment Shader", fragment_buffer_, sizeof(fragment_buffer_));

    if (ImGui::Button("Create")) {
        std::string name{name_buffer_};
        if (name.empty()) {
            draw_gui_.AddModalWindow(std::make_unique<WindowMessageBox>(
                "Error", "Invalid name."));
            return true;
        }
        if (level_.GetIdFromName(name) != frame::NullId) {
            draw_gui_.AddModalWindow(std::make_unique<WindowMessageBox>(
                "Error", "Name already used."));
            return true;
        }
        try {
            std::ifstream vert_file(frame::file::FindFile(
                std::string("asset/shader/opengl/") + vertex_buffer_));
            std::ifstream frag_file(frame::file::FindFile(
                std::string("asset/shader/opengl/") + fragment_buffer_));
            auto program = frame::opengl::CreateProgram(
                name,
                vertex_buffer_,
                fragment_buffer_,
                vert_file,
                frag_file);
            program->SetSerializeEnable(true);
            level_.AddProgram(std::move(program));
            if (update_json_callback_)
                update_json_callback_();
            end_ = true;
        } catch (const std::exception& e) {
            frame::Logger::GetInstance()->error(e.what());
            draw_gui_.AddModalWindow(
                std::make_unique<WindowMessageBox>("Error", e.what()));
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        end_ = true;
    }
    return true;
}

} // namespace frame::gui
