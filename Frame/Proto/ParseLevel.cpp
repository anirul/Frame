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
			auto maybe_cube_id = opengl::CreateCubeStaticMesh(this);
			if (!maybe_cube_id) 
				throw std::runtime_error("Could not create static cube mesh.");
			cube_id_ = maybe_cube_id.value();
			auto maybe_quad_id = opengl::CreateQuadStaticMesh(this);
			if (!maybe_quad_id)
				throw std::runtime_error("Could not create static quad mesh.");
			quad_id_ = maybe_quad_id.value();

			// Load textures from proto.
			for (const auto& proto_texture : proto_texture_file.textures())
			{
				std::optional<std::unique_ptr<TextureInterface>> maybe_texture;
				std::unique_ptr<TextureInterface> texture = nullptr;
				if (proto_texture.cubemap())
				{
					if (proto_texture.file_name().empty())
					{
						maybe_texture = 
							ParseCubeMapTexture(proto_texture, size);
					}
					else
					{
						maybe_texture = ParseCubeMapTextureFile(proto_texture);
					}	
				}
				else
				{
					if (proto_texture.file_names().empty())
						maybe_texture = ParseTexture(proto_texture, size);
					else
						maybe_texture = ParseTextureFile(proto_texture);
				}
				if (!maybe_texture)
				{
					throw std::runtime_error(
						fmt::format(
							"Could not load texture: {}",
							proto_texture.file_name()));
				}
				texture = std::move(maybe_texture.value());
				texture->SetName(proto_texture.name());
				if (!AddTexture(std::move(texture)))
				{
					throw std::runtime_error(
						fmt::format(
							"Coudn't save texture {} to level.", 
							proto_texture.name()));
				}
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
				auto maybe_program = ParseProgramOpenGL(proto_program, this);
				if (!maybe_program) 
				{
					throw std::runtime_error(
						fmt::format(
							"invalid program: {}", 
							proto_program.name()));
				}
				auto program = std::move(maybe_program.value());
				program->SetName(proto_program.name());
				if (!AddProgram(std::move(program)))
				{
					throw std::runtime_error(
						fmt::format(
							"Couldn't save program {} to level.",
							proto_program.name()));
				}
			}

			// Load material from proto.
			for (const auto& proto_material : proto_material_file.materials())
			{
				auto maybe_material = 
					ParseMaterialOpenGL(proto_material, this);
				if (!maybe_material)
				{
					throw std::runtime_error(
						fmt::format(
							"invalid material : {}",
							proto_material.name()));
				}
				auto material = std::move(maybe_material.value());
				material->SetName(proto_material.name());
				if (!AddMaterial(std::move(material)))
				{
					throw std::runtime_error(
						fmt::format(
							"Couldn't save material {} to level.",
							proto_material.name()));
				}
			}

			// Load scenes from proto.
			if (!ParseSceneTreeFile(proto_scene_tree_file, this))
				throw std::runtime_error("Could not parse proto scene file.");
			default_camera_name_ = proto_scene_tree_file.default_camera_name();
			if (default_camera_name_.empty())
				throw std::runtime_error("should have a default camera name.");
		}

	};

	std::optional<std::unique_ptr<LevelInterface>> ParseLevelOpenGL(
		const std::pair<std::int32_t, std::int32_t> size,
		const proto::Level& proto_level,
		const proto::ProgramFile& proto_program_file,
		const proto::SceneTreeFile& proto_scene_tree_file,
		const proto::TextureFile& proto_texture_file,
		const proto::MaterialFile& proto_material_file)
	{
		auto ptr = std::make_unique<LevelProto>(
			size,
			proto_level,
			proto_program_file,
			proto_scene_tree_file,
			proto_texture_file,
			proto_material_file);
		if (ptr) return ptr;
		return std::nullopt;
	}

} // End namespace frame::proto.
