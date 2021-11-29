#include "Frame/OpenGL/File/LoadTexture.h"
#include <algorithm>
#include <fstream>
#include <vector>
#include <set>
#include "Frame/File/FileSystem.h"
#include "Frame/File/Image.h"
#include "Frame/OpenGL/Material.h"
#include "Frame/OpenGL/Texture.h"
#include "Frame/OpenGL/TextureCubeMap.h"
#include "Frame/Proto/ParseLevel.h"
#include "Frame/Proto/ProtoLevelCreate.h"

namespace frame::opengl::file {

	namespace {

		std::set<std::string> basic_byte_extention = { "jpeg", "jpg" };
		std::set<std::string> basic_rgba_extention = { "png" };
		std::set<std::string> cube_map_half_extention = { "hdr", "dds" };

		proto::Level CreateEquirectangularProtoLevel()
		{
			proto::Level level{};
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
				proto::SceneCamera scene_camera{};
				{
					scene_camera.set_name("camera");
					scene_camera.set_parent("root");
					scene_camera.set_fov_degrees(90.0f);
				}
				*scene_tree_file.add_scene_cameras() = scene_camera;
				scene_tree_file.set_default_root_name("root");
				scene_tree_file.set_default_camera_name("camera");
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
				texture.set_name("InputTexture");
				texture.set_file_name(input_file);
				texture.set_cubemap(false);
				*texture.mutable_pixel_element_size() = pixel_element_size;
				*texture.mutable_pixel_structure() = pixel_structure;
				*texture_file.add_textures() = texture;
			}
			{
				proto::Texture texture{};
				texture.set_name("OutputCubemap");
				proto::Size proto_size{};
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
		std::string extention = file.substr(file.find_last_of(".") + 1);
		if (cube_map_half_extention.count(extention))
		{
			return LoadCubeMapTextureFromFile(
				file, 
				pixel_element_size, 
				pixel_structure);
		}
		else
		{
			return std::make_unique<frame::opengl::Texture>(
				image.GetSize(),
				image.Data(),
				pixel_element_size,
				pixel_structure);
		}
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
		auto maybe_level = frame::proto::ParseLevelOpenGL(
			{128, 128}, 
			CreateEquirectangularProtoLevel(),
			CreateEquirectangularProtoProgramFile(),
			CreateEquirectangularProtoSceneTreeFile(),
			CreateEquirectangularProtoTextureFile(
				file, 
				{128, 128}, 
				proto::PixelElementSize_HALF(),
				proto::PixelStructure_RGB()),
			CreateEquirectangularProtoMaterialFile());
		ScopedBind scoped_bind_frame(*frame);
		ScopedBind scoped_bind_render(*render);
		auto equirectangular = 
			LoadTextureFromFile(file, pixel_element_size, pixel_structure);
		
		throw std::runtime_error("Not implemented!");
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
