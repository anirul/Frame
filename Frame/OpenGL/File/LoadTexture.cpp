#include "Frame/OpenGL/File/LoadTexture.h"
#include <algorithm>
#include <fstream>
#include <vector>
#include <set>
#include "Frame/File/FileSystem.h"
#include "Frame/File/Image.h"
#include "Frame/OpenGL/Texture.h"

namespace frame::opengl::file {

	namespace {

		std::set<std::string> basic_byte_extention = { "jpeg", "jpg" };
		std::set<std::string> basic_rgba_extention = { "png" };
		std::set<std::string> cube_map_half_extention = { "hdr", "dds" };

	}

	std::shared_ptr<frame::TextureInterface> LoadTextureFromFile(
		const std::string& file, 
		const proto::PixelElementSize pixel_element_size 
			/*= proto::PixelElementSize_BYTE()*/, 
		const proto::PixelStructure pixel_structure 
			/*= proto::PixelStructure_RGB()*/)
	{
		std::shared_ptr<TextureInterface> texture = nullptr;
		frame::file::Image image(file, pixel_element_size, pixel_structure);
		std::string extention = file.substr(file.find_last_of(".") + 1);
		if (cube_map_half_extention.count(extention))
		{
			throw std::runtime_error("Not implemented yet!");
		}
		else
		{
			texture = std::make_shared<frame::opengl::Texture>(
				image.GetSize(),
				image.Data(),
				pixel_element_size,
				pixel_structure);
		}
		return texture;
	}

	std::shared_ptr<frame::TextureInterface> LoadCubeMapTextureFromFile(
		const std::string& file, 
		const proto::PixelElementSize pixel_element_size 
			/*= proto::PixelElementSize_BYTE()*/, 
		const proto::PixelStructure pixel_structure 
			/*= proto::PixelStructure_RGB()*/)
	{
		throw std::runtime_error("Not implemented!");
	}

	std::shared_ptr<frame::TextureInterface> LoadCubeMapTextureFromFiles(
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
		return std::make_shared<opengl::TextureCubeMap>(
			img_size,
			pointers,
			pixel_element_size,
			pixel_structure);
	}


	std::shared_ptr<TextureInterface> LoadTextureFromVec4(
		const glm::vec4& vec4)
	{
		std::array<float, 4> ar = { vec4.x,	vec4.y,	vec4.z,	vec4.w };
		return std::make_shared<frame::opengl::Texture>(
			std::make_pair<std::uint32_t, std::uint32_t>(1, 1),
			ar.data(),
			frame::proto::PixelElementSize_FLOAT(),
			frame::proto::PixelStructure_RGB_ALPHA());
	}

	std::shared_ptr<TextureInterface> LoadTextureFromFloat(float f)
	{
		return std::make_shared<frame::opengl::Texture>(
			std::make_pair<std::uint32_t, std::uint32_t>(1, 1),
			&f,
			frame::proto::PixelElementSize_FLOAT(),
			frame::proto::PixelStructure_GREY());
	}


} // End namespace frame::file.
