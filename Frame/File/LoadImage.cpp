#include "LoadImage.h"
#include <algorithm>
#include <fstream>
#include <vector>
#include <set>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "Frame/OpenGL/Texture.h"

namespace frame::file {

	Image::Image(
		const std::string& file,
		const proto::PixelElementSize pixel_element_size 
			/*= PixelElementSize::BYTE*/,
		const proto::PixelStructure pixel_structure /*= PixelStructure::RGB*/) :
		pixel_element_size_(pixel_element_size),
		pixel_structure_(pixel_structure)
	{
		int channels;
		int desired_channels = { static_cast<int>(pixel_structure.value()) };
		// This is in the case of OpenGL (for now the only case).
		stbi_set_flip_vertically_on_load(true);
		std::pair<int, int> size = { 0, 0 };
		switch (pixel_element_size.value())
		{
		case proto::PixelElementSize::BYTE :
		{
			image_ =
				stbi_load(
					file.c_str(),
					&size.first,
					&size.second,
					&channels,
					desired_channels);
			break;
		}
		case proto::PixelElementSize::SHORT :
		{
			image_ =
				stbi_load_16(
					file.c_str(),
					&size.first,
					&size.second,
					&channels,
					desired_channels);
			break;
		}
		case proto::PixelElementSize::HALF:
			[[fallthrough]];
		case proto::PixelElementSize::FLOAT :
		{
			image_ =
				stbi_loadf(
					file.c_str(),
					&size.first,
					&size.second,
					&channels,
					desired_channels);
			break;
		}
		default:
			throw
				std::runtime_error(
					"unsupported element size : " +
					std::to_string(
						static_cast<int>(pixel_element_size_.value())));
		}
		if (!image_)
		{
			throw std::runtime_error("unsupported file: " + file);
		}
		size_ = size;
	}

	Image::~Image()
	{
		stbi_image_free(image_);
	}

	namespace {

		std::set<std::string> basic_byte_extention = { "jpeg", "jpg" };
		std::set<std::string> basic_rgba_extention = { "png" };
		std::set<std::string> cube_map_half_extention = { "hdr", "dds" };

	}

	std::shared_ptr<frame::TextureInterface> LoadTextureFromFileOpenGL(
		const std::string& file, 
		const proto::PixelElementSize pixel_element_size 
			/*= proto::PixelElementSize_BYTE()*/, 
		const proto::PixelStructure pixel_structure 
			/*= proto::PixelStructure_RGB()*/)
	{
		std::shared_ptr<TextureInterface> texture = nullptr;
		Image image(file, pixel_element_size, pixel_structure);
		std::string extention = file.substr(file.find_last_of(".") + 1);
		if (cube_map_half_extention.count(extention))
		{
			texture = std::make_shared<frame::opengl::TextureCubeMap>(
				image.GetSize(),
				image.Data(),
				pixel_element_size,
				pixel_structure);
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

} // End namespace frame::file.
