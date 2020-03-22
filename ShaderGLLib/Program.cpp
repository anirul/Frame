#include "Program.h"
#include <stdexcept>

namespace sgl {

	Program::Program()
	{
		program_id_ = glCreateProgram();
		if (program_id_ <= 0)
		{
			throw std::runtime_error("Could not have a program that is <= 0");
		}
	}

	Program::~Program()
	{
		glDeleteProgram(program_id_);
	}

	void Program::AddShader(const Shader& shader)
	{
		glAttachShader(program_id_, shader.GetId());
	}

	void Program::LinkShader()
	{
		glLinkProgram(program_id_);
	}

	void Program::Use() const
	{
		glUseProgram(program_id_);
	}

	void Program::UniformBool(const std::string& name, bool value) const
	{
		glUniform1i(
			glGetUniformLocation(program_id_, name.c_str()), 
			(int)value);
	}

	void Program::UniformInt(const std::string& name, int value) const
	{
		glUniform1i(glGetUniformLocation(program_id_, name.c_str()), value);
	}

	void Program::UniformFloat(const std::string& name, float value) const
	{
		glUniform1f(glGetUniformLocation(program_id_, name.c_str()), value);
	}

	void Program::UniformVector2(
		const std::string& name, 
		const sgl::vector2& vec2) const
	{
		glUniform2f(
			glGetUniformLocation(program_id_, name.c_str()), 
			vec2.x, 
			vec2.y);
	}

	void Program::UniformVector3(
		const std::string& name, 
		const sgl::vector3& vec3) const
	{
		glUniform3f(
			glGetUniformLocation(program_id_, name.c_str()), 
			vec3.x, 
			vec3.y, 
			vec3.z);
	}

	void Program::UniformVector4(
		const std::string& name, 
		const sgl::vector4& vec4) const
	{
		glUniform4f(
			glGetUniformLocation(program_id_, name.c_str()),
			vec4.x,
			vec4.y,
			vec4.z,
			vec4.w);
	}

	void Program::UniformMatrix(
		const std::string& name, 
		const sgl::matrix& mat,
		const bool transpose /*= false*/) const
	{
		glUniformMatrix4fv(
			glGetUniformLocation(program_id_, name.c_str()), 
			1, 
			transpose ? GL_TRUE : GL_FALSE,
			&mat._11);
	}

} // End namespace sgl.
