#include "frame/opengl/file/load_program.h"

#include <frame/file/file_system.h>
#include <frame/opengl/program.h>

#include <filesystem>
#include <fstream>

namespace frame::opengl::file
{

// TODO(anirul): Should be moved to the device.
std::unique_ptr<frame::ProgramInterface> LoadProgram(
    const proto::Program& proto_program)
{
    std::string vertex_file =
        std::string("asset/shader/opengl/" + proto_program.shader() + ".vert");
    std::string fragment_file =
        std::string("asset/shader/opengl/" + proto_program.shader() + ".frag");
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
