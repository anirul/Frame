#pragma once

#include <GL/glew.h>

namespace frame::opengl {

/**
 * @brief Call back for error in the OpenGL pipeline.
 * @param source: From where in the code.
 * @param type: Type of error.
 * @param id: Error id.
 * @param severity: Error severity.
 * @param length: Length or the error message.
 * @param message: Error message.
 * @param userParam: Unused.
 */
void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                GLsizei length, const GLchar* message, const void* userParam);

} // End namespace frame::opengl.
