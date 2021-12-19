#include "Frame/OpenGL/File/LoadTexture.h"

#include <algorithm>
#include <fstream>
#include <vector>
#include <set>

#include "Frame/File/FileSystem.h"
#include "Frame/File/Image.h"
#include "Frame/Logger.h"
#include "Frame/NodeMatrix.h"
#include "Frame/OpenGL/Material.h"
#include "Frame/OpenGL/Renderer.h"
#include "Frame/OpenGL/StaticMesh.h"
#include "Frame/OpenGL/Texture.h"
#include "Frame/OpenGL/TextureCubeMap.h"
#include "Frame/Proto/ParseLevel.h"
#include "Frame/Proto/ProtoLevelCreate.h"

namespace frame::opengl::file {

	namespace {

		// Get the 6 view for the cube map.
		const std::array<glm::mat4, 6> views_cubemap =
		{
			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(1.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, -1.0f, 0.0f)),
			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(-1.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, -1.0f, 0.0f)),
			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 1.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, 1.0f)),
			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, -1.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, -1.0f)),
			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, 1.0f),
				glm::vec3(0.0f, -1.0f, 0.0f)),
			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, -1.0f),
				glm::vec3(0.0f, -1.0f, 0.0f))
		};

		std::set<std::string> byte_extention = { "jpeg", "jpg" };
		std::set<std::string> rgba_extention = { "png" };
		std::set<std::string> half_extention = { "hdr", "dds" };

		const std::string proto_level_json = R"json(
				{
					"name": "Equirectangular",
					"default_texture_name": "OutputCubemap"
				}
			)json";

		proto::Level CreateEquirectangularProtoLevel()
		{
			proto::Level level = proto::LoadProtoFromJson<proto::Level>(proto_level_json);
			level.set_name("Equirectangular");
			level.set_default_texture_name("OutputCubemap");
			return level;
		}

		proto::ProgramFile CreateEquirectangularProtoProgramFile()
		{
			proto::ProgramFile program_file{};
			proto::Program program{};
			{
				program.set_name("EquirectangularProgram");
				program.set_shader("EquirectangularCubeMap");
				proto::SceneType scene_type{};
				scene_type.set_value(proto::SceneType::QUAD);
				*program.mutable_input_scene_type() = scene_type;
				program.add_output_texture_names("OutputCubemap");
			}
			*program_file.add_programs() = program;
			return program_file;
		}

		proto::SceneTreeFile CreateEquirectangularProtoSceneTreeFile()
		{
			proto::SceneTreeFile scene_tree_file{};
			{
				proto::SceneMatrix scene_matrix{};
				{
					scene_matrix.set_name("root");
				}
				*scene_tree_file.add_scene_matrices() = scene_matrix;
				proto::SceneMatrix scene_camera_boon{};
				{
					scene_camera_boon.set_name("camera_boon");
					scene_camera_boon.set_parent("root");
				}
				*scene_tree_file.add_scene_matrices() = scene_camera_boon;
				proto::SceneCamera scene_camera{};
				{
					scene_camera.set_name("camera");
					scene_camera.set_parent("camera_boon");
					scene_camera.set_fov_degrees(90.0f);
					scene_camera.set_near_clip(0.1f);
					scene_camera.set_far_clip(1000.0f);
					scene_camera.set_aspect_ratio(1.0f);
				}
				*scene_tree_file.add_scene_cameras() = scene_camera;
				scene_tree_file.set_default_root_name("root");
				scene_tree_file.set_default_camera_name("camera");
				proto::SceneStaticMesh scene_static_mesh{};
				{
					scene_static_mesh.set_name("Cube");
					scene_static_mesh.set_mesh_enum(
						proto::SceneStaticMesh::CUBE);
					scene_static_mesh.set_material_name(
						"EquirectangularMaterial");
					scene_static_mesh.set_parent("root");
				}
				*scene_tree_file.add_scene_static_meshes() = scene_static_mesh;
			}
			return scene_tree_file;
		}

		proto::TextureFile CreateEquirectangularProtoTextureFile(
			const std::string& input_file,
			const std::pair<std::uint32_t, std::uint32_t> out_size,
			const proto::PixelElementSize pixel_element_size,
			const proto::PixelStructure pixel_structure)
		{
			proto::TextureFile texture_file{};
			{
				proto::Texture texture{};
				proto::Size proto_size{};
				texture.set_name("InputTexture");
				texture.set_file_name(input_file);
				texture.set_cubemap(false);
				proto_size.set_x(out_size.first);
				proto_size.set_y(out_size.second);
				*texture.mutable_size() = proto_size;
				*texture.mutable_pixel_element_size() = pixel_element_size;
				*texture.mutable_pixel_structure() = pixel_structure;
				*texture_file.add_textures() = texture;
			}
			{
				proto::Texture texture{};
				proto::Size proto_size{};
				texture.set_name("OutputCubemap");
				texture.set_cubemap(true);
				proto_size.set_x(out_size.first);
				proto_size.set_y(out_size.second);
				*texture.mutable_size() = proto_size;
				*texture.mutable_pixel_element_size() = pixel_element_size;
				*texture.mutable_pixel_structure() = pixel_structure;
				*texture_file.add_textures() = texture;
			}
			return texture_file;
		}

		proto::MaterialFile CreateEquirectangularProtoMaterialFile()
		{
			proto::MaterialFile material_file{};
			{
				proto::Material material{};
				material.set_name("EquirectangularMaterial");
				material.set_program_name("EquirectangularProgram");
				material.add_texture_names("InputTexture");
				material.add_inner_names("Equirectangular");
				*material_file.add_materials() = material;
			}
			return material_file;
		}

		std::uint32_t PowerFloor(std::uint32_t x) {
			std::uint32_t power = 1;
			while (x >>= 1) power <<= 1;
			return power;
		}
	}

	std::optional<std::unique_ptr<frame::TextureInterface>> 
	LoadTextureFromFile(
		const std::string& file, 
		const proto::PixelElementSize pixel_element_size 
			/*= proto::PixelElementSize_BYTE()*/, 
		const proto::PixelStructure pixel_structure 
			/*= proto::PixelStructure_RGB()*/)
	{
		frame::file::Image image(file, pixel_element_size, pixel_structure);	
		return std::make_unique<frame::opengl::Texture>(
			image.GetSize(),
			image.Data(),
			pixel_element_size,
			pixel_structure);
	}

	std::optional<std::unique_ptr<frame::TextureInterface>> 
	LoadCubeMapTextureFromFile(
		const std::string& file, 
		const proto::PixelElementSize pixel_element_size 
			/*= proto::PixelElementSize_BYTE()*/, 
		const proto::PixelStructure pixel_structure 
			/*= proto::PixelStructure_RGB()*/)
	{
		auto material = std::make_unique<frame::opengl::Material>();
		auto frame = std::make_unique<frame::opengl::FrameBuffer>();
		auto render = std::make_unique<frame::opengl::RenderBuffer>();
		auto& logger = Logger::GetInstance();
		auto maybe_equirectangular =
			LoadTextureFromFile(file, pixel_element_size, pixel_structure);
		if (!maybe_equirectangular)
		{
			logger->info("Could not load texture: [{}].", file);
			return std::nullopt;
		}
		std::unique_ptr<TextureInterface> equirectangular = 
			std::move(maybe_equirectangular.value());
		auto size = equirectangular->GetSize();
		// Seams correct when you are less than 2048 in height you get 512.
		std::uint32_t cube_single_res = PowerFloor(size.second) / 2;
		std::pair<std::uint32_t, std::uint32_t> cube_pair_res =
			{ cube_single_res, cube_single_res };
		auto maybe_level = frame::proto::ParseLevelOpenGL(
			cube_pair_res,
			CreateEquirectangularProtoLevel(),
			CreateEquirectangularProtoProgramFile(),
			CreateEquirectangularProtoSceneTreeFile(),
			CreateEquirectangularProtoTextureFile(
				file,
				cube_pair_res,
				proto::PixelElementSize_HALF(),
				proto::PixelStructure_RGB()),
			CreateEquirectangularProtoMaterialFile());
		if (!maybe_level)
		{
			logger->info("Could not create level.");
			return std::nullopt;
		}
		auto level = std::move(maybe_level.value());
		auto maybe_id = level->GetIdFromName("OutputCubemap");
		if (!maybe_id)
		{
			logger->info("Could not get the id of \"OutputCubemap\".");
			return std::nullopt;
		}
		auto* out_texture_ptr = level->GetTextureFromId(maybe_id.value());
		Renderer renderer(level.get(), cube_pair_res);
		auto maybe_camera_boon_id = level->GetIdFromName("camera_boon");
		if (!maybe_camera_boon_id) 
			throw std::runtime_error("Could not get camera boon id.");
		auto camera_boon_id = maybe_camera_boon_id.value();
		auto camera_boon = level->GetSceneNodeFromId(camera_boon_id);
		NodeMatrix* scene_matrix = dynamic_cast<NodeMatrix*>(camera_boon);
		if (!scene_matrix)
			throw std::runtime_error("Could not cast to NodeMatrix.");
		for (std::uint32_t i = 0; i < 6; ++i)
		{
			frame->AttachTexture(
				out_texture_ptr->GetId(),
				FrameColorAttachment::COLOR_ATTACHMENT0,
				0,
				static_cast<FrameTextureType>(i));
			scene_matrix->SetMatrix(views_cubemap[i]);
			renderer.RenderFromRootNode();
		}
		auto maybe_output_id = level->GetIdFromName("OutputCubemap");
		if (!maybe_output_id) return std::nullopt;
		return level->ExtractTexture(maybe_output_id.value());
	}

	std::optional<std::unique_ptr<frame::TextureInterface>> 
	LoadCubeMapTextureFromFiles(
		const std::array<std::string, 6> files, 
		const proto::PixelElementSize pixel_element_size 
			/*= proto::PixelElementSize_BYTE()*/, 
		const proto::PixelStructure pixel_structure 
			/*= proto::PixelStructure_RGB()*/)
	{
		std::array<std::string, 6> final_files = {};
		for (int i = 0; i < final_files.size(); ++i)
		{
			final_files[i] = frame::file::FindFile(files[i]);
		}
		std::pair<std::uint32_t, std::uint32_t> img_size;
		std::array<std::unique_ptr<frame::file::Image>, 6> images;
		std::array<void*, 6> pointers = {};
		for (int i = 0; i < pointers.size(); ++i)
		{
			images[i] = std::make_unique<frame::file::Image>(
				final_files[i],
				pixel_element_size,
				pixel_structure);
			pointers[i] = images[i]->Data();
		}
		img_size = images[0]->GetSize();
		return std::make_unique<opengl::TextureCubeMap>(
			img_size,
			pointers,
			pixel_element_size,
			pixel_structure);
	}


	std::optional<std::unique_ptr<TextureInterface>> 
	LoadTextureFromVec4(
		const glm::vec4& vec4)
	{
		std::array<float, 4> ar = { vec4.x,	vec4.y,	vec4.z,	vec4.w };
		return std::make_unique<frame::opengl::Texture>(
			std::make_pair<std::uint32_t, std::uint32_t>(1, 1),
			ar.data(),
			frame::proto::PixelElementSize_FLOAT(),
			frame::proto::PixelStructure_RGB_ALPHA());
	}

	std::optional<std::unique_ptr<TextureInterface>> 
	LoadTextureFromFloat(float f)
	{
		return std::make_unique<frame::opengl::Texture>(
			std::make_pair<std::uint32_t, std::uint32_t>(1, 1),
			&f,
			frame::proto::PixelElementSize_FLOAT(),
			frame::proto::PixelStructure_GREY());
	}

} // End namespace frame::file.
