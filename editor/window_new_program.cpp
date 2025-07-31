#include "window_new_program.h"

#include <filesystem>
#include <format>
#include <fstream>
#include <imgui.h>

#include "frame/file/file_system.h"
#include "frame/gui/window_message_box.h"
#include "frame/logger.h"
#include "frame/opengl/file/load_program.h"
#include "frame/opengl/program.h"

namespace frame::gui
{

WindowNewProgram::WindowNewProgram(
    DrawGuiInterface& draw_gui,
    LevelInterface& level,
    std::function<void()> update_json_callback)
    : draw_gui_(draw_gui), level_(level),
      update_json_callback_(std::move(update_json_callback))
{
}

bool WindowNewProgram::DrawCallback()
{
    ImGui::InputText("Name", name_buffer_, sizeof(name_buffer_));
    ImGui::InputText("Shader", shader_buffer_, sizeof(shader_buffer_));

    if (ImGui::Button("Create"))
    {
        std::string name{name_buffer_};
        if (name.empty())
        {
            draw_gui_.AddModalWindow(
                std::make_unique<WindowMessageBox>("Error", "Invalid name."));
            return true;
        }
        if (level_.GetIdFromName(name) != frame::NullId)
        {
            draw_gui_.AddModalWindow(
                std::make_unique<WindowMessageBox>(
                    "Error", "Name already used."));
            return true;
        }
        try
        {
            std::string shader{shader_buffer_};
            std::string vert_name = shader + ".vert";
            std::string frag_name = shader + ".frag";

            // Create default shader files if they do not exist.
            std::filesystem::path shader_dir =
                frame::file::FindDirectory("asset/shader/opengl/");
            std::filesystem::path vert_path = shader_dir / vert_name;
            std::filesystem::path frag_path = shader_dir / frag_name;
            if (!std::filesystem::exists(vert_path))
            {
                std::ofstream out(vert_path);
                out << "#version 330 core\n"
                       "layout(location = 0) in vec3 in_position;\n"
                       "void main()\n"
                       "{\n"
                       "    gl_Position = vec4(in_position, 1.0);\n"
                       "}\n";
            }
            if (!std::filesystem::exists(frag_path))
            {
                std::ofstream out(frag_path);
                out << "#version 330 core\n"
                       "out vec4 frag_color;\n"
                       "void main()\n"
                       "{\n"
                       "    frag_color = vec4(1.0);\n"
                       "}\n";
            }

            std::ifstream vert_file(vert_path);
            std::ifstream frag_file(frag_path);
            auto program = frame::opengl::CreateProgram(
                name, vert_name, frag_name, vert_file, frag_file);
            program->SetSceneRoot(level_.GetDefaultRootSceneNodeId());
            program->SetSerializeEnable(true);
            level_.AddProgram(std::move(program));
            if (update_json_callback_)
                update_json_callback_();
            end_ = true;
        }
        catch (const std::exception& e)
        {
            frame::Logger::GetInstance()->error(e.what());
            draw_gui_.AddModalWindow(
                std::make_unique<WindowMessageBox>("Error", e.what()));
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
    {
        end_ = true;
    }
    return true;
}

} // namespace frame::gui
