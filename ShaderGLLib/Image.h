#pragma once

#include <vector>
#include <string>
#include "Vector.h"

namespace sgl {

	class Image
	{
	public:
		Image(const std::string& file);
		virtual ~Image();

	public:
		const std::pair<size_t, size_t> GetSize() const { return { dx_, dy_ }; }
		const float GetWidth() const { return static_cast<float>(dx_); }
		const float GetHeight() const { return static_cast<float>(dy_); }
		const std::uint8_t* Data() const { return image_; }

	private:
		size_t dx_ = 0;
		size_t dy_ = 0;
		std::uint8_t* image_;
	};

}	// End of namespace sgl.
