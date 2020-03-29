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

	private:
		size_t dx_ = 0;
		size_t dy_ = 0;
	};

}	// End of namespace sgl.
