#include "ParseProgram.h"
#include "Frame/OpenGL/Program.h"

namespace frame::proto {

	std::shared_ptr<frame::ProgramInterface> ParseProgramOpenGL(
		const Program& proto_program,
		const std::map<std::string, std::uint64_t>& name_id_textures)
	{
		auto program = frame::opengl::CreateProgram(proto_program.shader());
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
