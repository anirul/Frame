#include "ParseLevel.h"
#include "Frame/LevelBase.h"
#include "Frame/OpenGL/Texture.h"
#include "Frame/OpenGL/Material.h"
#include "Frame/ProgramInterface.h"
#include "Frame/Proto/ParseMaterial.h"
#include "Frame/Proto/ParseProgram.h"
#include "Frame/Proto/ParseSceneTree.h"
#include "Frame/Proto/ParseTexture.h"
#include "Frame/SceneTree.h"

namespace frame::proto {

	class LevelOpenGL :
		public LevelBase,
		public std::enable_shared_from_this<LevelOpenGL>
	{
	public:
		LevelOpenGL(
			const std::pair<std::int32_t, std::int32_t> size,
			const proto::Level& proto_level,
			const proto::ProgramFile& proto_program_file,
			const proto::SceneTreeFile& proto_scene_tree_file,
			const proto::TextureFile& proto_texture_file,
			const proto::MaterialFile& proto_material_file)
		{
			name_ = proto_level.name();
			default_texture_name_ = proto_level.default_texture_name();
			default_scene_name_ = proto_level.default_scene_name();
			default_camera_name_ = proto_level.default_camera_name();
			if (default_scene_name_.empty())
				throw std::runtime_error("should have a default scene name.");
			if (default_texture_name_.empty())
				throw std::runtime_error("should have a default texture.");
			if (default_camera_name_.empty())
				throw std::runtime_error("should have a default camera.");

			// Load scenes from proto.
			frame::proto::SceneTree proto_scene_tree;
			for (const auto& proto : proto_scene_tree_file.scene_trees())
			{
				if (proto.name() == default_scene_name_)
					proto_scene_tree = proto;
			}
			scene_tree_ = ParseSceneTreeOpenGL(proto_scene_tree);
			scene_tree_->SetDefaultCamera(default_camera_name_);

			// Load textures from proto.
			std::map<std::string, std::uint64_t> name_id_textures;
			for (const auto& proto_texture : proto_texture_file.textures())
			{
				std::shared_ptr<TextureInterface> texture = nullptr;
				if (proto_texture.cubemap())
					texture = proto::ParseCubeMapTexture(proto_texture, size);
				else
					texture = proto::ParseTexture(proto_texture, size);
				std::uint64_t id = GetTextureNewId();
				id_texture_map_.insert({ id, texture });
				name_id_map_.insert({ proto_texture.name(), id });
				id_name_map_.insert({ id, proto_texture.name() });
				name_id_textures.insert({ proto_texture.name(), id });
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
				std::shared_ptr<ProgramInterface> program =
					proto::ParseProgramOpenGL(proto_program, name_id_textures);
				std::uint64_t id = GetProgramNewId();
				id_program_map_.insert({ id, program });
				name_id_map_.insert({ proto_program.name(), id });
				id_name_map_.insert({ id, proto_program.name() });
			}

			// Load material from proto.
			for (const auto& proto_material : proto_material_file.materials())
			{
				std::shared_ptr<MaterialInterface> material =
					proto::ParseMaterialOpenGL(
						proto_material,
						shared_from_this());
				std::uint64_t id = GetMaterialNewId();
				id_material_map_.insert({ id, material });
				name_id_map_.insert({ proto_material.name(), id });
				id_name_map_.insert({ id, proto_material.name() });
			}
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
		return std::make_shared<LevelOpenGL>(
			size,
			proto_level,
			proto_program_file,
			proto_scene_tree_file,
			proto_texture_file,
			proto_material_file);
	}

} // End namespace frame::proto.
