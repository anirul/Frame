#include "frame/open_gl/file/load_program.h"

#include <fstream>

#include "frame/file/file_system.h"
#include "frame/open_gl/program.h"

namespace frame::opengl::file {

std::optional<std::unique_ptr<frame::ProgramInterface>> LoadProgram(const std::string& name) {
    std::string vertex_file   = frame::file::FindFile("asset/shader/open_gl/" + name + ".vert");
    std::string fragment_file = frame::file::FindFile("asset/shader/open_gl/" + name + ".frag");
    return LoadProgram(vertex_file, fragment_file);
}

std::optional<std::unique_ptr<frame::ProgramInterface>> LoadProgram(
    const std::string& vertex_file, const std::string& fragment_file) {
    std::ifstream vertex_ifs{ frame::file::FindFile(vertex_file) };
    std::ifstream fragment_ifs{ frame::file::FindFile(fragment_file) };
    return CreateProgram(vertex_ifs, fragment_ifs);
}

}  // namespace frame::opengl::file
