#pragma once

#include <cstdint>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "../FrameProto/Proto.h"

namespace sgl {

	using PixelElementSize = frame::proto::PixelElementSize;
	PixelElementSize PixelElementSize_BYTE();
	PixelElementSize PixelElementSize_SHORT();
	PixelElementSize PixelElementSize_HALF();
	PixelElementSize PixelElementSize_FLOAT();
	bool operator==(const PixelElementSize& l, const PixelElementSize& r);

	using PixelStructure = frame::proto::PixelStructure;
	PixelStructure PixelStructure_GREY();
	PixelStructure PixelStructure_GREY_ALPHA();
	PixelStructure PixelStructure_RGB();
	PixelStructure PixelStructure_RGB_ALPHA();
	bool operator==(const PixelStructure& l, const PixelStructure& r);
	
	// Get the GL_FLOAT and GL_UNSIGNED_BYTE.
	int ConvertToGLType(
		const PixelElementSize& pixel_element_size);
	// Get the GL_RGBA or GL_R.
	int ConvertToGLType(
		const PixelStructure& pixel_structure);
	// Get the GL_RGBA8 and GL_RG32F.
	int ConvertToGLType(
		const PixelElementSize& pixel_element_size, 
		const PixelStructure& pixel_structure);

} // End namespace sgl.