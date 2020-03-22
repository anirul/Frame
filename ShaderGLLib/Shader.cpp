#include "Shader.h"

#include <fstream>
#include <iostream>

namespace sgl {

	Shader::~Shader()
	{
		if (created_)
		{
			glDeleteShader(id_);
		}
	}

	bool Shader::LoadFromSource(const std::string& source)
	{
		id_ = glCreateShader(static_cast<unsigned int>(type_));
		created_ = true;
		const char* c_source = source.c_str();
		glShaderSource(id_, 1, &c_source, nullptr);
		glCompileShader(id_);
		int result;
		glGetShaderiv(id_, GL_COMPILE_STATUS, &result);
		if (result == GL_FALSE)
		{
			int length;
			glGetShaderiv(id_, GL_INFO_LOG_LENGTH, &length);
			error_message_.resize(length);
			glGetShaderInfoLog(id_, length, &length, &error_message_[0]);
			glDeleteShader(id_);
			created_ = false;
			return false;
		}
		error_message_ = "";
		return true;
	}

	bool Shader::LoadFromFile(const std::string& path)
	{
		std::ifstream ifs;
		ifs.open(path, std::ios::in);
		if (!ifs.is_open()) {
			error_message_ = "Could not open file: " + path;
			return false;
		}
		std::string str(std::istreambuf_iterator<char>(ifs), {});
		return LoadFromSource(str);
	}

}	// End namespace sgl.
