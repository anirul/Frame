#include "ParseLevel.h"
#include "Frame/Level.h"
#include "Frame/OpenGL/Texture.h"
#include "Frame/OpenGL/Material.h"
#include "Frame/OpenGL/StaticMesh.h"
#include "Frame/ProgramInterface.h"
#include "Frame/Proto/ParseMaterial.h"
#include "Frame/Proto/ParseProgram.h"
#include "Frame/Proto/ParseSceneTree.h"
#include "Frame/Proto/ParseTexture.h"

namespace frame::proto {

	class LevelProto :	public frame::Level
	{
	public:
		LevelProto(
			const std::pair<std::int32_t, std::int32_t> size,
			const proto::Level& proto_level,
			const proto::ProgramFile& proto_program_file,
			const proto::SceneTreeFile& proto_scene_tree_file,
			const proto::TextureFile& proto_texture_file,
			const proto::MaterialFile& proto_material_file)
		{
			name_ = proto_level.name();
			default_texture_name_ = proto_level.default_texture_name();
			if (default_texture_name_.empty())
				throw std::runtime_error("should have a default texture.");

			// Include the default cube and quad.
			cube_id_ = opengl::CreateCubeStaticMesh(this);
			quad_id_ = opengl::CreateQuadStaticMesh(this);

			// Load textures from proto.
			for (const auto& proto_texture : proto_texture_file.textures())
			{
				std::shared_ptr<TextureInterface> texture = nullptr;
				if (proto_texture.cubemap())
				{
					if (proto_texture.file_name().empty())
						texture = ParseCubeMapTexture(proto_texture, size);
					else
						texture = ParseCubeMapTextureFile(proto_texture);
				}
				else
				{
					if (proto_texture.file_names().empty())
						texture = ParseTexture(proto_texture, size);
					else
						texture = ParseTextureFile(proto_texture);
				}
				auto texture_id = AddTexture(proto_texture.name(), texture);
			}

			// Check the default texture is in.
			if (name_id_map_.find(default_texture_name_) == name_id_map_.end())
			{
				throw std::runtime_error(
					"no default texture is loaded: " + default_texture_name_);
			}

			// Load programs from proto.
			for (const auto& proto_program : proto_program_file.programs())
			{
				auto program = ParseProgramOpenGL(proto_program, this);
				AddProgram(proto_program.name(), program);
			}

			// Load material from proto.
			for (const auto& proto_material : proto_material_file.materials())
			{
				auto material = ParseMaterialOpenGL(proto_material, this);
				AddMaterial(proto_material.name(), material);
			}

			// Load scenes from proto.
			ParseSceneTreeFile(proto_scene_tree_file, this);
			default_camera_name_ = proto_scene_tree_file.default_camera_name();
			if (default_camera_name_.empty())
				throw std::runtime_error("should have a default camera name.");
		}

	};

	std::shared_ptr<LevelInterface> ParseLevelOpenGL(
		const std::pair<std::int32_t, std::int32_t> size,
		const proto::Level& proto_level,
		const proto::ProgramFile& proto_program_file,
		const proto::SceneTreeFile& proto_scene_tree_file,
		const proto::TextureFile& proto_texture_file,
		const proto::MaterialFile& proto_material_file)
	{
		return std::make_shared<LevelProto>(
			size,
			proto_level,
			proto_program_file,
			proto_scene_tree_file,
			proto_texture_file,
			proto_material_file);
	}

} // End namespace frame::proto.
