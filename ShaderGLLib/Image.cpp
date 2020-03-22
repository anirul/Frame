#include "Image.h"

#include <algorithm>
#include <fstream>
#include <vector>
#include <tuple>
#include <assert.h>

namespace sgl {

	Image::Image(const std::string& file)
	{
		if (!LoadFromTGA(file))
		{
			throw std::runtime_error("couldn't load from file: " + file);
		}
	}

	bool Image::CheckHeader(const TgaHeader& header)
	{
		if ((header.length != 0) && (header.length != sizeof(header)))
		{
			return false;
		}
		if ((header.bits != 8) &&
			(header.bits != 16) &&
			(header.bits != 24) &&
			(header.bits != 32))
		{
			return false;
		}
		if (header.width > 8192 || header.height > 8192)
		{
			return false;
		}
		return true;
	}

	bool Image::LoadFromTGA(const std::string& path)
	{
		std::ifstream ifs(path, std::ios::binary);
		if (!ifs.is_open()) return false;

		ifs.seekg(0, std::ios_base::end);
		size_t file_size = ifs.tellg();
		ifs.seekg(0, std::ios_base::beg);

		TgaHeader header;
		// Fill up the header.
		ifs.read((char*)&header.length, sizeof(uint8_t));
		ifs.read((char*)&header.color_map_type, sizeof(uint8_t));
		ifs.read((char*)&header.image_type, sizeof(uint8_t));
		ifs.read((char*)&header.color_map_origin, sizeof(uint16_t));
		ifs.read((char*)&header.color_map_length, sizeof(uint16_t));
		ifs.read((char*)&header.color_map_entry_size, sizeof(uint8_t));
		ifs.read((char*)&header.x_origin, sizeof(uint16_t));
		ifs.read((char*)&header.y_origin, sizeof(uint16_t));
		ifs.read((char*)&header.width, sizeof(uint16_t));
		ifs.read((char*)&header.height, sizeof(uint16_t));
		ifs.read((char*)&header.bits, sizeof(uint8_t));
		ifs.read((char*)&header.image_descriptor, sizeof(uint8_t));
		if (!CheckHeader(header))
		{
			return false;
		}
		dx_ = header.width;
		dy_ = header.height;
		// Resize the buffer.
		resize(dx_ * dy_);
		// Check the content.
		switch (header.image_type) 
		{
		case 2:
		{
			if (header.bits == 24)
			{
				std::for_each(begin(), end(), [&ifs](sgl::vector& v)
				{
					uint8_t r;
					uint8_t g;
					uint8_t b;
					ifs.read((char*)&b, sizeof(uint8_t));
					ifs.read((char*)&g, sizeof(uint8_t));
					ifs.read((char*)&r, sizeof(uint8_t));
					v.x = static_cast<float>(r) / 255.f;
					v.y = static_cast<float>(g) / 255.f;
					v.z = static_cast<float>(b) / 255.f;
					v.w = 1.f;
				});
			}
			else if (header.bits == 32)
			{
				std::for_each(begin(), end(), [&ifs](sgl::vector& v)
				{
					uint8_t r;
					uint8_t g;
					uint8_t b;
					uint8_t a;
					// CHECKME: not sure of the order.
					ifs.read((char*)&b, sizeof(uint8_t));
					ifs.read((char*)&g, sizeof(uint8_t));
					ifs.read((char*)&r, sizeof(uint8_t));
					ifs.read((char*)&a, sizeof(uint8_t));
					v.x = static_cast<float>(r) / 255.f;
					v.y = static_cast<float>(g) / 255.f;
					v.z = static_cast<float>(b) / 255.f;
					v.w = static_cast<float>(a) / 255.f;
				});
			}
			else 
			{ 
				return false; 
			}
			break;
		}
		default:
			return false;
		}
		return true;
	}

}	// End namespace sgl.
