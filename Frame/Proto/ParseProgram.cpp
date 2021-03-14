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
		std::string shader_name = 
			file::FindPath("Asset/") + "Shader/OpenGL/" + proto_program.shader();
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
		return program;
	}

} // End namespace frame::proto.
