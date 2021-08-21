#pragma once

#include <array>
#include <vector>
#include <string>
#include <memory>
#include "Frame/OpenGL/Pixel.h"
#include "Frame/Proto/ParsePixel.h"
#include "Frame/TextureInterface.h"

namespace frame::opengl::file {


	std::unique_ptr<TextureInterface> LoadTextureFromVec4(
		const glm::vec4& vec4);

	std::unique_ptr<TextureInterface> LoadTextureFromFloat(float f);

	std::unique_ptr<TextureInterface> LoadTextureFromFile(
		const std::string& file,
		const proto::PixelElementSize pixel_element_size =
			proto::PixelElementSize_BYTE(),
		const proto::PixelStructure pixel_structure =
			proto::PixelStructure_RGB());

	// Not implemented yet!
	std::unique_ptr<TextureInterface> LoadCubeMapTextureFromFile(
		const std::string& file,
		const proto::PixelElementSize pixel_element_size =
			proto::PixelElementSize_BYTE(),
		const proto::PixelStructure pixel_structure =
			proto::PixelStructure_RGB());

	std::unique_ptr<TextureInterface> LoadCubeMapTextureFromFiles(
		const std::array<std::string, 6> files,
		const proto::PixelElementSize pixel_element_size =
			proto::PixelElementSize_BYTE(),
		const proto::PixelStructure pixel_structure =
			proto::PixelStructure_RGB());

}	// End of namespace frame::file.
