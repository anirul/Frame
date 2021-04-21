#include "ParseProgram.h"
#include <fstream>
#include "Frame/OpenGL/Program.h"
#include "Frame/File/FileSystem.h"

namespace frame::proto {

	std::shared_ptr<frame::ProgramInterface> ParseProgramOpenGL(
		const Program& proto_program,
		const LevelInterface* level)
	{
		Error& error = Error::GetInstance();
		Logger& logger = Logger::GetInstance();
		std::string shader_path = "Asset/Shader/OpenGL/";
		std::string shader_name = shader_path + proto_program.shader();
		std::string shader_vert = file::FindFile(shader_name + ".vert");
		logger->info("Openning vertex shader: [{}].", shader_vert);
		std::ifstream ifs_vertex(shader_vert);
		if (!ifs_vertex.is_open())
		{
			std::string error_str = 
				fmt::format("Couldn't open file {}.vert", shader_name);
			error.CreateError(error_str, __FILE__, __LINE__ - 4);
		}
		std::string shader_frag = file::FindFile(shader_name + ".frag");
		logger->info("Openning fragment shader: [{}].", shader_frag);
		std::ifstream ifs_pixel(shader_frag);
		if (!ifs_pixel.is_open())
		{
			std::string error_str =
				fmt::format("Couldn't open file {}.frag", shader_name);
			error.CreateError(error_str, __FILE__, __LINE__ - 4);
		}
		auto program = opengl::CreateProgram(ifs_vertex, ifs_pixel);
		for (const auto& texture_name : proto_program.input_texture_names())
		{
			EntityId texture_id = level->GetIdFromName(texture_name);
			// Check this is a texture.
			(void)level->GetTextureMap().at(texture_id);
			program->AddInputTextureId(texture_id);
		}
		for (const auto& texture_name : proto_program.output_texture_names())
		{
			EntityId texture_id = level->GetIdFromName(texture_name);
			// Check this is a texture.
			(void)level->GetTextureMap().at(texture_id);
			program->AddOutputTextureId(texture_id);
		}
		program->SetSceneRoot(0);
		switch (proto_program.input_scene_type().value())
		{
		case SceneType::QUAD:
		{
			EntityId quad_id = level->GetDefaultStaticMeshQuadId();
			program->SetSceneRoot(quad_id);
			break;
		}
		case SceneType::CUBE:
		{
			EntityId cube_id = level->GetDefaultStaticMeshCubeId();
			program->SetSceneRoot(cube_id);
			break;
		}
		case SceneType::SCENE:
		{
			EntityId scene_id = 
				level->GetIdFromName(proto_program.input_scene_name());
			program->SetSceneRoot(scene_id);
			break;
		}
		case SceneType::NONE:
		default:
			throw std::runtime_error(
				fmt::format(
					"No way {}?", 
					proto_program.input_scene_type().value()));
		}
		program->SetDepthTest(proto_program.depth_test());
		for (const auto& parameter : proto_program.parameters())
		{
			program->Uniform(parameter.name(), parameter.uniform_enum());
		}
		return program;
	}

} // End namespace frame::proto.
