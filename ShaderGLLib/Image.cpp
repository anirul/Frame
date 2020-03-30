#include "Image.h"

#include <algorithm>
#include <fstream>
#include <vector>
#include <tuple>
#include <assert.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace sgl {

	Image::Image(const std::string& file)
	{
		int channels;
		int width;
		int height;
		int desired_channels = { STBI_rgb_alpha };
		image_ = 
			stbi_load(
				file.c_str(), 
				&width, 
				&height, 
				&channels, 
				desired_channels);
		if (!image_)
		{
			throw std::runtime_error("couldn't load from file: " + file);
		}
		dx_ = static_cast<size_t>(width);
		dy_ = static_cast<size_t>(height);		
	}

	Image::~Image()
	{
		stbi_image_free(image_);
	}

}	// End namespace sgl.
