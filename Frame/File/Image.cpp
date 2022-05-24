#include "Frame/File/Image.h"
#include <algorithm>
#include <fstream>
#include <vector>
#include <set>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
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
		const auto& logger = frame::Logger::GetInstance();
		logger->info("Openning image: [{}].", file);
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
		free_ = true;
		size_ = size;
	}

	Image::Image(
		std::pair<std::uint32_t, std::uint32_t> size,
		const proto::PixelElementSize pixel_element_size/* =
			proto::PixelElementSize_BYTE()*/,
		const proto::PixelStructure pixel_structure/* =
		proto::PixelStructure_RGB()*/) :
			size_(size),
			pixel_element_size_(pixel_element_size),
			pixel_structure_(pixel_structure)
	{
		free_ = false;
	}

	void Image::SaveImageToFile(const std::string& file) const
	{
		// For OpenGL it seams...
		stbi_flip_vertically_on_write(true);
		const auto& logger = frame::Logger::GetInstance();
		logger->info("Saving [{}]...", file);
		if (!image_) throw std::runtime_error("no pointer to be saved?");
		stbi_write_png(
			file.c_str(),
			size_.first,
			size_.second,
			pixel_structure_.value(),
			image_,
			size_.first * pixel_structure_.value());
	}

	void Image::SetData(void* data)
	{
		if (free_) stbi_image_free(image_);
		image_ = data;
		free_ = false;
	}

	Image::~Image()
	{
		if (free_) stbi_image_free(image_);
	}

} // End namespace frame::file.
