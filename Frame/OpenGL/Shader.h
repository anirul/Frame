#pragma once

#include <string>
#include <GL/glew.h>

namespace frame::opengl {

	enum class ShaderEnum 
	{
		VERTEX_SHADER = GL_VERTEX_SHADER,
		FRAGMENT_SHADER = GL_FRAGMENT_SHADER,
		GEOMETRY_SHADER = GL_GEOMETRY_SHADER,
	};

	class Shader 
	{
	public:
		Shader(const ShaderEnum type) : type_(type) {}
		virtual ~Shader();

	public:
		bool LoadFromSource(const std::string& source);
		bool LoadFromFile(const std::string& path);

	public:
		unsigned int GetId() const { return id_; }
		const std::string GetErrorMessage() const { return error_message_; }

	private:
		bool created_ = false;
		unsigned int id_ = 0;
		ShaderEnum type_ = ShaderEnum::VERTEX_SHADER;
		std::string error_message_;
	};

}	// End namespace frame::opengl.
