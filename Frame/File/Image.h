#pragma once

#include <array>
#include <vector>
#include <string>
#include <memory>
#include "Frame/ImageInterface.h"
#include "Frame/Proto/ParsePixel.h"

namespace frame::file {

	class Image : public ImageInterface
	{
	public:
		Image(
			std::pair<std::uint32_t, std::uint32_t> size,
			const proto::PixelElementSize pixel_element_size =
			proto::PixelElementSize_BYTE(),
			const proto::PixelStructure pixel_structure =
			proto::PixelStructure_RGB());
		Image(
			const std::string& file, 
			const proto::PixelElementSize pixel_element_size = 
				proto::PixelElementSize_BYTE(),
			const proto::PixelStructure pixel_structure = 
				proto::PixelStructure_RGB());
		void SaveImageToFile(const std::string& file) const;
		virtual ~Image();

	public:
		const std::pair<std::uint32_t, std::uint32_t> GetSize() const override 
		{ 
			return size_; 
		}
		const int GetLength() const override 
		{ 
			return size_.first * size_.second; 
		}
		void SetData(void* data);
		const void* Data() const override { return image_; }
		// Needed for the accessing of the pointer.
		void* Data() override { return image_; }
		const proto::PixelElementSize GetPixelElementSize() const override 
		{
			return pixel_element_size_;
		}
		const proto::PixelStructure GetPixelStructure() const override 
		{
			return pixel_structure_;
		}

	private:
		std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
		bool free_ = false;
		void* image_ = nullptr;
		const proto::PixelElementSize pixel_element_size_;
		const proto::PixelStructure pixel_structure_;
	};

}	// End of namespace frame::file.
