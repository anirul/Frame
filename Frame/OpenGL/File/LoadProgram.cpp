#include "Frame/OpenGL/File/LoadProgram.h"
#include <fstream>
#include "Frame/File/FileSystem.h"
#include "Frame/OpenGL/Program.h"

namespace frame::opengl::file {

	std::shared_ptr<frame::ProgramInterface> LoadProgram(
		const std::string& name)
	{
		std::string vertex_file = 
			frame::file::FindFile("Asset/Shader/OpenGL/" + name + ".vert");
		std::string fragment_file =
			frame::file::FindFile("Asset/Shader/OpenGL/" + name + ".frag");
		return LoadProgram(vertex_file, fragment_file);
	}

	std::shared_ptr<frame::ProgramInterface> LoadProgram(
		const std::string& vertex_file,
		const std::string& fragment_file)
	{
		std::ifstream vertex_ifs{ frame::file::FindFile(vertex_file) };
		std::ifstream fragment_ifs{ frame::file::FindFile(fragment_file) };
		return CreateProgram(vertex_ifs, fragment_ifs);
	}

} // End namespace frame::file.
