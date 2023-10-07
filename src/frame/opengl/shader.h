#pragma once

#include <GL/glew.h>

#include <string>

namespace frame::opengl
{

enum class ShaderEnum
{
    VERTEX_SHADER = GL_VERTEX_SHADER,
    FRAGMENT_SHADER = GL_FRAGMENT_SHADER,
    GEOMETRY_SHADER = GL_GEOMETRY_SHADER,
};

/**
 * @class Shader
 * @brief Shader class to hold the shader and make the compilation and
 *        parsing of it.
 */
class Shader
{
  public:
    /**
     * @brief Constructor that create a shader from a type (no source no
     *        compilation yet).
     * @param type: Shader type see the enum.
     */
    Shader(const ShaderEnum type) : type_(type)
    {
    }
    /**
     *  @brief Destructor this is where the memory is freed and so is the
     *         shader.
     */
    virtual ~Shader();

  public:
    /**
     * @brief Load a from a shader source.
     * @param source: Content of the shader in text form.
     */
    bool LoadFromSource(const std::string& source);

  public:
    /**
     * @brief Get the underlying id from OpenGL.
     * @return The id from OpenGL.
     */
    unsigned int GetId() const
    {
        return id_;
    }
    /**
     * @brief Get the error message from compilation of the shader.
     * @return The error message from compilation of the shader.
     */
    const std::string GetErrorMessage() const
    {
        return error_message_;
    }

  private:
    bool created_ = false;
    unsigned int id_ = 0;
    ShaderEnum type_ = ShaderEnum::VERTEX_SHADER;
    std::string error_message_;
};

} // End namespace frame::opengl.
