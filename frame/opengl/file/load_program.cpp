#include "frame/opengl/file/load_program.h"

#include <frame/file/file_system.h>
#include <frame/json/program_catalog.h>
#include <frame/opengl/program.h>

#include <filesystem>
#include <fstream>
#include <format>
#include <stdexcept>

namespace frame::opengl::file
{

// TODO(anirul): Should be moved to the device.
std::unique_ptr<frame::ProgramInterface> LoadProgram(
    const proto::Program& proto_program)
{
    auto shader_files = frame::json::ResolveProgramShaderFiles(
        proto_program,
        frame::json::ShaderBackend::OpenGL);
    if (!shader_files)
    {
        throw std::runtime_error(
            std::format(
                "No OpenGL pipeline mapping found for program '{}' (pipeline '{}').",
                proto_program.name(),
                proto_program.has_pipeline_name()
                    ? proto_program.pipeline_name()
                    : proto_program.name()));
    }
    std::string vertex_file =
        std::string("asset/shader/opengl/" + shader_files->vertex_shader);
    std::string fragment_file =
        std::string("asset/shader/opengl/" + shader_files->fragment_shader);
    return LoadProgram(proto_program, vertex_file, fragment_file);
}

// TODO(anirul): Should be moved to the device.
std::unique_ptr<frame::ProgramInterface> LoadProgram(
    const proto::Program& proto_program,
    const std::string& vertex_file,
    const std::string& fragment_file)
{
    std::ifstream vertex_ifs{
        frame::file::FindFile(std::filesystem::path(vertex_file))};
    std::ifstream fragment_ifs{
        frame::file::FindFile(std::filesystem::path(fragment_file))};
    return CreateProgram(proto_program, vertex_ifs, fragment_ifs);
}

} // namespace frame::opengl::file
