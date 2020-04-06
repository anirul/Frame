#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace sgl {

	// Simple enum to hold the size of an element of a pixel.
	enum class PixelElementSize : std::uint8_t 
	{
		BYTE = 1,
		SHORT = 2,
		LONG = 4,
	};

	// Simple enum to hold the structure of a pixel.
	enum class PixelStructure : std::uint8_t
	{
		GREY = 1,
		GREY_ALPHA = 2,
		RGB = 3,
		RGB_ALPHA = 4,
	};

	// Get the GL_FLOAT and GL_UNSIGNED_BYTE.
	int ConvertToGLType(const PixelElementSize pixel_element_size);
	// Get the GL_RGBA or GL_R.
	int ConvertToGLType(const PixelStructure pixel_structure);
	// Get the GL_RGBA8 and GL_RG32F.
	int ConvertToGLType(
		const PixelElementSize pixel_element_size, 
		const PixelStructure pixel_structure);

} // End namespace sgl.