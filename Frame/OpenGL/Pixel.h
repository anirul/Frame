#pragma once

#include <cstdint>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "Frame/Proto/Proto.h"

namespace frame::opengl {

	// Get the GL_FLOAT and GL_UNSIGNED_BYTE.
	GLenum ConvertToGLType(
		const frame::proto::PixelElementSize& pixel_element_size);
	// Get the GL_RGBA or GL_R.
	GLenum ConvertToGLType(
		const frame::proto::PixelStructure& pixel_structure);
	// Get the GL_RGBA8 and GL_RG32F.
	GLenum ConvertToGLType(
		const frame::proto::PixelElementSize& pixel_element_size,
		const frame::proto::PixelStructure& pixel_structure);

} // End namespace frame::opengl.