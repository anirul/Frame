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
		std::uint8_t* img = 
			stbi_load(
				file.c_str(), 
				&width, 
				&height, 
				&channels, 
				desired_channels);
		if (!img)
		{
			throw std::runtime_error("couldn't load from file: " + file);
		}
		dx_ = static_cast<size_t>(width);
		dy_ = static_cast<size_t>(height);
		reserve(dx_ * dy_);
		for (int i = 0; i < dx_ * dy_; ++i) 
		{
			sgl::vector v;
			const int index = i * desired_channels;
			v.x = static_cast<float>(img[index + 0]) / 255.0f;
			v.y = static_cast<float>(img[index + 1]) / 255.0f;
			v.z = static_cast<float>(img[index + 2]) / 255.0f;
			v.w = static_cast<float>(img[index + 3]) / 255.0f;
			emplace_back(v);
		}
		stbi_image_free(img);
	}

}	// End namespace sgl.
