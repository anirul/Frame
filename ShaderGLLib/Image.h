#pragma once

#include <vector>
#include <string>
#include <memory>
#include "../ShaderGLLib/Pixel.h"

namespace sgl {

	class Image
	{
	public:
		Image(
			const std::string& file, 
			const PixelElementSize pixel_element_size = PixelElementSize::BYTE,
			const PixelStructure pixel_structure = PixelStructure::RGB_ALPHA);
		virtual ~Image();

	public:
		const std::pair<int, int> GetSize() const { return size_; }
		const void* Data() const {	return image_; }

	private:
		std::pair<int, int> size_ = { 0, 0 };
		void* image_ = nullptr;
	};

}	// End of namespace sgl.
