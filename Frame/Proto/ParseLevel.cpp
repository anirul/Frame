#include "ParseLevel.h"
#include "Frame/LevelBase.h"
#include "Frame/OpenGL/Texture.h"
#include "Frame/OpenGL/Material.h"
#include "Frame/ProgramInterface.h"
#include "Frame/Proto/ParseMaterial.h"
#include "Frame/Proto/ParseProgram.h"
#include "Frame/Proto/ParseSceneTree.h"
#include "Frame/Proto/ParseTexture.h"

namespace frame::proto {

	class LevelOpenGL :	public LevelBase
	{
	public:
		LevelOpenGL(
			const std::pair<std::int32_t, std::int32_t> size,
			const std::string& default_path,
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

			// Load scenes from proto.
			ParseSceneTreeFile(proto_scene_tree_file, this);
			default_camera_name_ = proto_scene_tree_file.default_camera_name();
			if (default_camera_name_.empty())
				throw std::runtime_error("should have a default camera name.");

			// Load textures from proto.
			std::map<std::string, std::uint64_t> name_id_textures;
			for (const auto& proto_texture : proto_texture_file.textures())
			{
				std::shared_ptr<TextureInterface> texture = nullptr;
				if (proto_texture.cubemap())
					texture = ParseCubeMapTexture(proto_texture, size);
				else
					texture = ParseTexture(proto_texture, size);
				auto texture_id = AddTexture(proto_texture.name(), texture);
				name_id_textures.insert({ proto_texture.name(), texture_id });
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
				auto program = ParseProgramOpenGL(
					proto_program, 
					default_path,
					name_id_textures);
				AddProgram(proto_program.name(), program);
			}

			// Load material from proto.
			for (const auto& proto_material : proto_material_file.materials())
			{
				std::shared_ptr<MaterialInterface> material =
					proto::ParseMaterialOpenGL(proto_material, this);
				AddMaterial(proto_material.name(), material);
			}
		}
	};

	std::shared_ptr<LevelInterface> ParseLevelOpenGL(
		const std::pair<std::int32_t, std::int32_t> size,
		const std::string& default_path,
		const proto::Level& proto_level,
		const proto::ProgramFile& proto_program_file,
		const proto::SceneTreeFile& proto_scene_tree_file,
		const proto::TextureFile& proto_texture_file,
		const proto::MaterialFile& proto_material_file)
	{
		return std::make_shared<LevelOpenGL>(
			size,
			default_path,
			proto_level,
			proto_program_file,
			proto_scene_tree_file,
			proto_texture_file,
			proto_material_file);
	}

} // End namespace frame::proto.
