#include "Program.h"
#include <stdexcept>
#include "Error.h"

namespace sgl {

	Program::Program()
	{
		program_id_ = glCreateProgram();
		error_.Display(__FILE__, __LINE__ - 1);
	}

	Program::~Program()
	{
		glDeleteProgram(program_id_);
	}

	void Program::AddShader(const Shader& shader)
	{
		glAttachShader(program_id_, shader.GetId());
		error_.Display(__FILE__, __LINE__ - 1);
		attached_shaders_.push_back(shader.GetId());
	}

	void Program::LinkShader()
	{
		glLinkProgram(program_id_);
		error_.Display(__FILE__, __LINE__ - 1);
		for (const auto& id : attached_shaders_)
		{
			glDetachShader(program_id_, id);
			error_.Display(__FILE__, __LINE__ - 1);
		}
	}

	void Program::Use() const
	{
		glUseProgram(program_id_);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Program::UniformBool(const std::string& name, bool value) const
	{
		glUniform1i(GetMemoizeUniformLocation(name), (int)value);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Program::UniformInt(const std::string& name, int value) const
	{
		glUniform1i(GetMemoizeUniformLocation(name), value);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Program::UniformFloat(const std::string& name, float value) const
	{
		glUniform1f(GetMemoizeUniformLocation(name), value);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Program::UniformVector2(
		const std::string& name, 
		const glm::vec2& vec2) const
	{
		glUniform2f(GetMemoizeUniformLocation(name), vec2.x, vec2.y);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Program::UniformVector3(
		const std::string& name, 
		const glm::vec3& vec3) const
	{
		glUniform3f(GetMemoizeUniformLocation(name), vec3.x, vec3.y, vec3.z);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Program::UniformVector4(
		const std::string& name, 
		const glm::vec4& vec4) const
	{
		glUniform4f(
			GetMemoizeUniformLocation(name),
			vec4.x,
			vec4.y,
			vec4.z,
			vec4.w);
		error_.Display(__FILE__, __LINE__ - 6);
	}

	void Program::UniformMatrix(
		const std::string& name, 
		const glm::mat4& mat,
		const bool transpose /*= false*/) const
	{
		glUniformMatrix4fv(
			GetMemoizeUniformLocation(name),
			1, 
			transpose ? GL_TRUE : GL_FALSE,
			&mat[0][0]);
		error_.Display(__FILE__, __LINE__ - 5);
	}

	const int Program::GetMemoizeUniformLocation(const std::string& name) const
	{
		auto it = memoize_map_.find(name);
		if (it == memoize_map_.end())
		{
			memoize_map_[name] = 
				glGetUniformLocation(program_id_, name.c_str());
			error_.Display(__FILE__, __LINE__ - 1);
		}
		return memoize_map_[name];
	}

	std::shared_ptr<Program> CreateProgram(const std::string& name)
	{
		auto program = std::make_shared<sgl::Program>();
		const auto& error = Error::GetInstance();
		sgl::Shader vertex(sgl::ShaderType::VERTEX_SHADER);
		sgl::Shader fragment(sgl::ShaderType::FRAGMENT_SHADER);
		if (!vertex.LoadFromFile("../Asset/Shader/" + name + ".vert"))
		{
			error.CreateError(
				vertex.GetErrorMessage(), 
				__FILE__, 
				__LINE__ - 5);
		}
		if (!fragment.LoadFromFile("../Asset/Shader/" + name + ".frag"))
		{
			error.CreateError(
				fragment.GetErrorMessage(), 
				__FILE__, 
				__LINE__ - 5);
		}
		program->AddShader(vertex);
		program->AddShader(fragment);
		program->LinkShader();
		program->Use();
		program->UniformMatrix("projection", glm::mat4(1.0));
		program->UniformMatrix("view", glm::mat4(1.0));
		program->UniformMatrix("model", glm::mat4(1.0));
		return program;
	}

} // End namespace sgl.
