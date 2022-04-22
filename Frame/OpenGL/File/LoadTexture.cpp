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

		const std::set<std::string> byte_extention = { "jpeg", "jpg" };
		const std::set<std::string> rgba_extention = { "png" };
		const std::set<std::string> half_extention = { "hdr", "dds" };

		const std::string proto_level_json = R"json(
				{
					"name": "Equirectangular",
					"default_texture_name": "OutputTexture",
					"programs": [
						{
							"name": "EquirectangularProgram",
							"shader": "EquirectangularCubemap",
							"input_scene_type":	{
								"value": "QUAD"
							},
							"output_texture_names": [ "OutputTexture" ]
						}
					],
					"scene_tree": {
						"default_root_name": "root",
						"default_camera_name": "camera",
						"scene_matrices": [
							{
								"name": "root",
								"clean_buffer": {
									"values": [ "CLEAR_COLOR", "CLEAR_DEPTH" ] 
								}
							},
							{
								"name": "camera_boon",
								"parent": "root"
							}
						],
						"scene_static_meshes": [
							{
								"name": "Cube",
								"mesh_enum": "CUBE",
								"material_name": "EquirectangularMaterial",
								"parent": "root"
							}
						],
						"scene_cameras": [
							{
								"name": "camera",
								"parent": "camera_boon",
								"fov_degrees": "90.0",
								"near_clip": "0.1",
								"far_clip": "1000.0",
								"aspect_ratio": "1.0"
							}
						]
					},
					"textures" : [
						{
							"name": "InputTexture",
							"file_name": "<filename>",
							"cubemap": "false",
							"size" : {
								"x": "<x>",
								"y": "<y>"
							},
							"pixel_element_size": { 
								"value": "<pixel_element_size>" 
							},
							"pixel_structure": { 
								"value": "<pixel_structure>" 
							}
						},
						{
							"name": "OutputTexture",
							"cubemap": "true",
							"size": {
								"x": "<x>",
								"y": "<y>"
							},
							"pixel_element_size": { 
								"value": "<pixel_element_size>" 
							},
							"pixel_structure": { 
								"value": "<pixel_structure>" 
							}
						}
					],
					"materials": [
						{
							"name": "EquirectangularMaterial",
							"program_name": "EquirectangularProgram",
							"texture_names": [ "InputTexture" ],
							"inner_names": "Equirectangular"
						}
					]
				}
			)json";

		// Taken from cpp reference.
        std::size_t replace_all(
			std::string& inout, 
			const std::string_view what, 
			const std::string_view with)
        {
            std::size_t count = 0;
            for (std::string::size_type pos = 0;
                inout.npos != 
					(pos = inout.find(what.data(), pos, what.length()));
                pos += with.length(), ++count) 
			{
                inout.replace(pos, what.length(), with.data(), with.length());
            }
            return count;
        }

		std::string FillLevel(
			const std::string& initial, 
			const std::map<std::string, std::string>& map)
		{

			std::string out = initial;
			for (auto [from, to] : map)
			{
				replace_all(to, "\\", "/");
				replace_all(out, from, to);
			}
			return out;
		}

		std::uint32_t PowerFloor(std::uint32_t x) {
			std::uint32_t power = 1;
			while (x >>= 1) power <<= 1;
			return power;
		}
	}

	std::unique_ptr<frame::TextureInterface> LoadTextureFromFile(
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

	std::unique_ptr<frame::TextureInterface> LoadCubeMapTextureFromFile(
		const std::string& file, 
		const proto::PixelElementSize pixel_element_size 
			/*= proto::PixelElementSize_BYTE()*/, 
		const proto::PixelStructure pixel_structure 
			/*= proto::PixelStructure_RGB()*/)
	{
		auto material = std::make_unique<frame::opengl::Material>();
		auto& logger = Logger::GetInstance();
		auto equirectangular =
			LoadTextureFromFile(file, pixel_element_size, pixel_structure);
		if (!equirectangular)
		{
			logger->info("Could not load texture: [{}].", file);
			return nullptr;
		}
		auto size = equirectangular->GetSize();
		// Seams correct when you are less than 2048 in height you get 512.
		std::uint32_t cube_single_res = PowerFloor(size.second) / 2;
		std::pair<std::uint32_t, std::uint32_t> cube_pair_res =
			{ cube_single_res, cube_single_res };
		std::map<std::string, std::string> filling_map = {
			{ "<filename>", file },
			{ "<x>", std::to_string(cube_pair_res.first) },
			{ "<y>", std::to_string(cube_pair_res.second) },
			{ 
				"<pixel_element_size>", 
				PixelElementSize_Enum_Name(pixel_element_size.value()) 
			},
			{ "<pixel_structure>", "RGB" }
		};
		auto maybe_level = 
			frame::proto::ParseLevelOpenGL(
				cube_pair_res,
				proto::LoadProtoFromJson<proto::Level>(
					FillLevel(proto_level_json, filling_map)));
		if (!maybe_level)
		{
			logger->info("Could not create level.");
			return nullptr;
		}
		auto level = std::move(maybe_level.value());
		auto maybe_id = level->GetIdFromName("OutputTexture");
		if (!maybe_id)
		{
			logger->info("Could not get the id of \"OutputTexture\".");
			return nullptr;
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
			renderer.SetCubeMapTarget(GetTextureFrameFromPosition(i));
			scene_matrix->SetMatrix(views_cubemap[i]);
			renderer.RenderFromRootNode();
		}
		auto maybe_output_id = level->GetIdFromName("OutputTexture");
		if (!maybe_output_id) return nullptr;
		return level->ExtractTexture(maybe_output_id.value());
	}

	std::unique_ptr<frame::TextureInterface> LoadCubeMapTextureFromFiles(
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


	std::unique_ptr<TextureInterface> LoadTextureFromVec4(const glm::vec4& vec4)
	{
		std::array<float, 4> ar = { vec4.x,	vec4.y,	vec4.z,	vec4.w };
		return std::make_unique<frame::opengl::Texture>(
			std::make_pair<std::uint32_t, std::uint32_t>(1, 1),
			ar.data(),
			frame::proto::PixelElementSize_FLOAT(),
			frame::proto::PixelStructure_RGB_ALPHA());
	}

	std::unique_ptr<TextureInterface> LoadTextureFromFloat(float f)
	{
		return std::make_unique<frame::opengl::Texture>(
			std::make_pair<std::uint32_t, std::uint32_t>(1, 1),
			&f,
			frame::proto::PixelElementSize_FLOAT(),
			frame::proto::PixelStructure_GREY());
	}

} // End namespace frame::file.
