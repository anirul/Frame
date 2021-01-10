#pragma once

#include <array>
#include <vector>
#include <string>
#include <memory>
#include "Frame/OpenGL/Pixel.h"
#include "Frame/Proto/ParsePixel.h"
#include "Frame/TextureInterface.h"

namespace frame::file {

	class Image
	{
	public:
		Image(
			const std::string& file, 
			const proto::PixelElementSize pixel_element_size = 
				proto::PixelElementSize_BYTE(),
			const proto::PixelStructure pixel_structure = 
				proto::PixelStructure_RGB());
		virtual ~Image();

	public:
		const std::pair<std::uint32_t, std::uint32_t> GetSize() const 
		{ 
			return size_; 
		}
		const int GetLength() const { return size_.first * size_.second; }
		const void* Data() const {	return image_; }
		const proto::PixelElementSize GetPixelElementSize() const
		{
			return pixel_element_size_;
		}
		const proto::PixelStructure GetPixelStructure() const
		{
			return pixel_structure_;
		}

	private:
		std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
		void* image_ = nullptr;
		const proto::PixelElementSize pixel_element_size_;
		const proto::PixelStructure pixel_structure_;
	};

	std::shared_ptr<TextureInterface> LoadTextureFromFileOpenGL(
		const std::string& file,
		const proto::PixelElementSize pixel_element_size =
			proto::PixelElementSize_BYTE(),
		const proto::PixelStructure pixel_structure =
			proto::PixelStructure_RGB());

	std::shared_ptr<TextureInterface> LoadCubeMapTextureFromFileOpenGL(
		const std::string& file,
		const proto::PixelElementSize pixel_element_size =
			proto::PixelElementSize_BYTE(),
		const proto::PixelStructure pixel_structure =
			proto::PixelStructure_RGB());

	std::shared_ptr<TextureInterface> LoadCubeMapTextureFromFilesOpenGL(
		const std::array<std::string, 6> files,
		const proto::PixelElementSize pixel_element_size =
			proto::PixelElementSize_BYTE(),
		const proto::PixelStructure pixel_structure =
			proto::PixelStructure_RGB());

}	// End of namespace frame::file.
