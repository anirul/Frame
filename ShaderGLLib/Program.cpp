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
		attached_shaders_.push_back(shader.GetId());
	}

	void Program::LinkShader()
	{
		glLinkProgram(program_id_);
		for (const auto& id : attached_shaders_)
		{
			glDetachShader(program_id_, id);
		}
	}

	void Program::Use() const
	{
		glUseProgram(program_id_);
	}

	void Program::UniformBool(const std::string& name, bool value) const
	{
		glUniform1i(GetMemoizeUniformLocation(name), (int)value);
	}

	void Program::UniformInt(const std::string& name, int value) const
	{
		glUniform1i(GetMemoizeUniformLocation(name), value);
	}

	void Program::UniformFloat(const std::string& name, float value) const
	{
		glUniform1f(GetMemoizeUniformLocation(name), value);
	}

	void Program::UniformVector2(
		const std::string& name, 
		const glm::vec2& vec2) const
	{
		glUniform2f(GetMemoizeUniformLocation(name), vec2.x, vec2.y);
	}

	void Program::UniformVector3(
		const std::string& name, 
		const glm::vec3& vec3) const
	{
		glUniform3f(GetMemoizeUniformLocation(name), vec3.x, vec3.y, vec3.z);
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
	}

	const int Program::GetMemoizeUniformLocation(const std::string& name) const
	{
		auto it = memoize_map_.find(name);
		if (it == memoize_map_.end())
		{
			memoize_map_[name] = 
				glGetUniformLocation(program_id_, name.c_str());
		}
		return memoize_map_[name];
	}

	std::shared_ptr<sgl::Program> CreateSimpleProgram(
		const glm::mat4& projection /*= glm::mat4(1.0f)*/, 
		const glm::mat4& view /*= glm::mat4(1.0f)*/,
		const glm::mat4& model /*= glm::mat4(1.0f)*/)
	{
		auto program = std::make_shared<sgl::Program>();
		sgl::Shader vertex(sgl::ShaderType::VERTEX_SHADER);
		sgl::Shader fragment(sgl::ShaderType::FRAGMENT_SHADER);
		vertex.LoadFromFile("../Asset/Simple.Vertex.glsl");
		fragment.LoadFromFile("../Asset/Simple.Fragment.glsl");
		program->AddShader(vertex);
		program->AddShader(fragment);
		program->LinkShader();
		program->Use();
		program->UniformMatrix("projection", projection);
		program->UniformMatrix("view", view);
		program->UniformMatrix("model", model);
		return program;
	}

	std::shared_ptr<sgl::Program> CreateCubeMapProgram(
		const glm::mat4& projection /*= glm::mat4(1.0f)*/,
		const glm::mat4& view /*= glm::mat4(1.0f)*/,
		const glm::mat4& model /*= glm::mat4(1.0f)*/)
	{
		auto program = std::make_shared<sgl::Program>();
		sgl::Shader vertex(sgl::ShaderType::VERTEX_SHADER);
		sgl::Shader fragment(sgl::ShaderType::FRAGMENT_SHADER);
		vertex.LoadFromFile("../Asset/CubeMap.Vertex.glsl");
		fragment.LoadFromFile("../Asset/CubeMap.Fragment.glsl");
		program->AddShader(vertex);
		program->AddShader(fragment);
		program->LinkShader();
		program->Use();
		program->UniformMatrix("projection", projection);
		program->UniformMatrix("view", view);
		program->UniformMatrix("model", model);
		return program;
	}

	std::shared_ptr<sgl::Program> CreatePBRProgram(
		const glm::mat4& projection /*= glm::mat4(1.0f)*/,
		const glm::mat4& view /*= glm::mat4(1.0f)*/,
		const glm::mat4& model /*= glm::mat4(1.0f)*/)
	{
		auto program = std::make_shared<sgl::Program>();
		sgl::Shader vertex(sgl::ShaderType::VERTEX_SHADER);
		sgl::Shader fragment(sgl::ShaderType::FRAGMENT_SHADER);
		vertex.LoadFromFile("../Asset/PBR.Vertex.glsl");
		fragment.LoadFromFile("../Asset/PBR.Fragment.glsl");
		program->AddShader(vertex);
		program->AddShader(fragment);
		program->LinkShader();
		program->Use();
		program->UniformMatrix("projection", projection);
		program->UniformMatrix("view", view);
		program->UniformMatrix("model", model);
		return program;
	}

} // End namespace sgl.
