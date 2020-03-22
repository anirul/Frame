#pragma once

#include <vector>
#include <string>
#include "Vector.h"

namespace sgl {

	class Image : public std::vector<sgl::vector>
	{
	public:
		Image(const std::string& file);

	public:
		const std::pair<size_t, size_t> GetSize() const 
		{ 
			return std::make_pair(dx_, dy_); 
		}
		const float GetWidth() const { return static_cast<float>(dx_); }
		const float GetHeight() const { return static_cast<float>(dy_); }

	protected:
		struct TgaHeader
		{
			uint8_t length;
			uint8_t color_map_type;
			uint8_t image_type;

			uint16_t color_map_origin;
			uint16_t color_map_length;

			uint8_t color_map_entry_size;

			uint16_t x_origin;
			uint16_t y_origin;
			uint16_t width;
			uint16_t height;

			uint8_t bits;
			uint8_t image_descriptor;
		};
		bool LoadFromTGA(const std::string& path);
		bool CheckHeader(const TgaHeader& header);

	private:
		size_t dx_ = 0;
		size_t dy_ = 0;
	};

}	// End of namespace sgl.
