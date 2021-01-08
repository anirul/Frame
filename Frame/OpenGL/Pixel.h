#pragma once

#include <cstdint>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "Frame/Proto/Proto.h"

namespace frame::opengl {

	// TODO(anirul): Move this to the proto part?
	frame::proto::PixelElementSize PixelElementSize_BYTE();
	frame::proto::PixelElementSize PixelElementSize_SHORT();
	frame::proto::PixelElementSize PixelElementSize_HALF();
	frame::proto::PixelElementSize PixelElementSize_FLOAT();
	bool operator==(
		const frame::proto::PixelElementSize& l, 
		const frame::proto::PixelElementSize& r);

	// TODO(anirul): Move this to the proto part?
	frame::proto::PixelStructure PixelStructure_GREY();
	frame::proto::PixelStructure PixelStructure_GREY_ALPHA();
	frame::proto::PixelStructure PixelStructure_RGB();
	frame::proto::PixelStructure PixelStructure_RGB_ALPHA();
	bool operator==(
		const frame::proto::PixelStructure& l, 
		const frame::proto::PixelStructure& r);
	
	// Get the GL_FLOAT and GL_UNSIGNED_BYTE.
	int ConvertToGLType(
		const frame::proto::PixelElementSize& pixel_element_size);
	// Get the GL_RGBA or GL_R.
	int ConvertToGLType(
		const frame::proto::PixelStructure& pixel_structure);
	// Get the GL_RGBA8 and GL_RG32F.
	int ConvertToGLType(
		const frame::proto::PixelElementSize& pixel_element_size,
		const frame::proto::PixelStructure& pixel_structure);

} // End namespace frame::opengl.