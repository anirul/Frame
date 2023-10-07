#pragma once

#include <GL/glew.h>

#include "frame/json/proto.h"

namespace frame::opengl
{

/**
 * @brief Get the GL_FLOAT and GL_UNSIGNED_BYTE.
 * @param pixel_element_size: Insert a pixel element size form proto.
 * @return The OpenGL corresponding value.
 */
GLenum ConvertToGLType(
    const frame::proto::PixelElementSize& pixel_element_size);
/**
 * @brief Get the GL_RGBA or GL_R.
 * @param pixel_structure: Insert the pixel structure from proto.
 * @return The OpenGL corresponding value.
 */
GLenum ConvertToGLType(const frame::proto::PixelStructure& pixel_structure);
/**
 * @brief Get the GL_RGBA8 and GL_RG32F.
 * @param pixel_element_size: Insert a pixel element size form proto.
 * @param pixel_structure: Insert the pixel structure from proto.
 * @return The OpenGL corresponding value.
 */
GLenum ConvertToGLType(
    const frame::proto::PixelElementSize& pixel_element_size,
    const frame::proto::PixelStructure& pixel_structure);

} // End namespace frame::opengl.
