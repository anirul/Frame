#include "Image.h"
#include <algorithm>
#include <fstream>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace sgl {

	Image::Image(
		const std::string& file, 
		const PixelElementSize pixel_element_size /*= PixelElementSize::BYTE*/, 
		const PixelStructure pixel_structure /*= PixelStructure::RGB_ALPHA*/) :
		pixel_element_size_(pixel_element_size),
		pixel_structure_(pixel_structure)
	{
		int channels;
		int desired_channels = { static_cast<int>(pixel_structure) };
		switch (pixel_element_size)
		{
		case PixelElementSize::BYTE :
		{
			image_ = 
				stbi_load(
					file.c_str(),
					&size_.first,
					&size_.second,
					&channels,
					desired_channels);
			break;
		}
		case PixelElementSize::SHORT :
		{
			image_ =
				stbi_load_16(
					file.c_str(),
					&size_.first,
					&size_.second,
					&channels,
					desired_channels);
			break;
		}
		case PixelElementSize::LONG :
		{
			image_ =
				stbi_loadf(
					file.c_str(),
					&size_.first,
					&size_.second,
					&channels,
					desired_channels);
			break;
		}
		default:
			throw 
				std::runtime_error(
					"unsupported element size : " +
					static_cast<int>(pixel_element_size_));
		}
		if (!image_)
		{
			throw std::runtime_error("unsupported file: " + file);
		}
	}

	Image::~Image()
	{
		stbi_image_free(image_);
	}

}	// End namespace sgl.
