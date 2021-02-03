#include "ParseProgram.h"
#include <fstream>
#include "Frame/OpenGL/Program.h"

namespace frame::proto {

	std::shared_ptr<frame::ProgramInterface> ParseProgramOpenGL(
		const Program& proto_program,
		const std::string& default_path,
		const std::map<std::string, std::uint64_t>& name_id_textures)
	{
		Error& error = Error::GetInstance();
		std::string shader_name = 
			default_path + "Shader/OpenGL/" + proto_program.shader();
		std::ifstream ifs_vertex(shader_name + ".vert");
		if (!ifs_vertex.is_open())
		{
			std::string error_str = 
				fmt::format("Couldn't open file {}", shader_name + ".vert");
			error.CreateError(error_str, __FILE__, __LINE__);
		}
		std::ifstream ifs_pixel(shader_name + ".frag");
		if (!ifs_pixel.is_open())
		{
			std::string error_str =
				fmt::format("Couldn't open file {}", shader_name + ".frag");
			error.CreateError(error_str, __FILE__, __LINE__);
		}
		auto program = opengl::CreateProgram(ifs_vertex, ifs_pixel);
		for (const auto& texture_name : proto_program.input_texture_names())
		{
			program->AddInputTextureId(name_id_textures.at(texture_name));
		}
		for (const auto& texture_name : proto_program.output_texture_names())
		{
			program->AddOutputTextureId(name_id_textures.at(texture_name));
		}
		return program;
	}

} // End namespace frame::proto.
