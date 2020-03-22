#pragma once

#include <string>
#include <GL/glew.h>

namespace sgl {

	enum class ShaderType {
		VERTEX_SHADER = GL_VERTEX_SHADER,
		FRAGMENT_SHADER = GL_FRAGMENT_SHADER,
		GEOMETRY_SHADER = GL_GEOMETRY_SHADER,
	};

	class Shader {
	public:
		Shader(const ShaderType type) : type_(type) {}
		virtual ~Shader();
		bool LoadFromSource(const std::string& source);
		bool LoadFromFile(const std::string& path);
		unsigned int GetId() const { return id_; }
		const std::string GetErrorMessage() const { return error_message_; }

	private:
		bool created_ = false;
		unsigned int id_ = 0;
		ShaderType type_ = ShaderType::VERTEX_SHADER;
		std::string error_message_;
	};

}	// End namespace sgl.
