#include "ParseProgram.h"
#include <fstream>
#include "Frame/OpenGL/Program.h"

namespace frame::proto {

	std::shared_ptr<frame::ProgramInterface> ParseProgramOpenGL(
		const Program& proto_program,
		const std::string& default_path,
		const std::map<std::string, std::uint64_t>& name_id_textures)
	{
		std::ifstream ifs_vertex(
			default_path + "Shader/OpenGL/" + proto_program.shader() + ".vert");
		std::ifstream ifs_pixel(
			default_path + "Shader/OpenGL/" + proto_program.shader() + ".frag");
		auto program = frame::opengl::CreateProgram(ifs_vertex, ifs_pixel);
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
