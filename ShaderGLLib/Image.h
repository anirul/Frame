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
			const PixelStructure pixel_structure = PixelStructure::RGB);
		virtual ~Image();

	public:
		const std::pair<int, int> GetSize() const { return size_; }
		const int GetLength() const { return size_.first * size_.second; }
		const void* Data() const {	return image_; }
		const PixelElementSize GetPixelElementSize() const
		{
			return pixel_element_size_;
		}
		const PixelStructure GetPixelStructure() const
		{
			return pixel_structure_;
		}

	private:
		std::pair<int, int> size_ = { 0, 0 };
		void* image_ = nullptr;
		const PixelElementSize pixel_element_size_;
		const PixelStructure pixel_structure_;
	};

}	// End of namespace sgl.
