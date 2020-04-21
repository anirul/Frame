#include "Program.h"
#include <stdexcept>
#include "Error.h"

namespace sgl {

	Program::Program()
	{
		program_id_ = glCreateProgram();
		error_->Display(__FILE__, __LINE__ - 1);
	}

	Program::~Program()
	{
		glDeleteProgram(program_id_);
	}

	void Program::AddShader(const Shader& shader)
	{
		glAttachShader(program_id_, shader.GetId());
		error_->DisplayShader(shader.GetId(), __FILE__, __LINE__ - 1);
		attached_shaders_.push_back(shader.GetId());
	}

	void Program::LinkShader()
	{
		glLinkProgram(program_id_);
		error_->Display(__FILE__, __LINE__ - 1);
		for (const auto& id : attached_shaders_)
		{
			glDetachShader(program_id_, id);
			error_->Display(__FILE__, __LINE__ - 1);
		}
	}

	void Program::Use() const
	{
		glUseProgram(program_id_);
		error_->Display(__FILE__, __LINE__ - 1);
	}

	void Program::UniformBool(const std::string& name, bool value) const
	{
		glUniform1i(GetMemoizeUniformLocation(name), (int)value);
		error_->Display(__FILE__, __LINE__ - 1);
	}

	void Program::UniformInt(const std::string& name, int value) const
	{
		glUniform1i(GetMemoizeUniformLocation(name), value);
		error_->Display(__FILE__, __LINE__ - 1);
	}

	void Program::UniformFloat(const std::string& name, float value) const
	{
		glUniform1f(GetMemoizeUniformLocation(name), value);
		error_->Display(__FILE__, __LINE__ - 1);
	}

	void Program::UniformVector2(
		const std::string& name, 
		const glm::vec2& vec2) const
	{
		glUniform2f(GetMemoizeUniformLocation(name), vec2.x, vec2.y);
		error_->Display(__FILE__, __LINE__ - 1);
	}

	void Program::UniformVector3(
		const std::string& name, 
		const glm::vec3& vec3) const
	{
		glUniform3f(GetMemoizeUniformLocation(name), vec3.x, vec3.y, vec3.z);
		error_->Display(__FILE__, __LINE__ - 1);
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
		error_->Display(__FILE__, __LINE__ - 6);
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
		error_->Display(__FILE__, __LINE__ - 5);
	}

	const int Program::GetMemoizeUniformLocation(const std::string& name) const
	{
		auto it = memoize_map_.find(name);
		if (it == memoize_map_.end())
		{
			memoize_map_[name] = 
				glGetUniformLocation(program_id_, name.c_str());
			error_->Display(__FILE__, __LINE__ - 1);
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
		vertex.LoadFromFile("../Asset/Simple.vert");
		fragment.LoadFromFile("../Asset/Simple.frag");
		program->AddShader(vertex);
		program->AddShader(fragment);
		program->LinkShader();
		program->Use();
		program->UniformMatrix("projection", projection);
		program->UniformMatrix("view", view);
		program->UniformMatrix("model", model);
		return program;
	}

	std::shared_ptr<sgl::Program> CreateRayMarchingProgram(
		const glm::mat4& projection /*= glm::mat4(1.0f)*/, 
		const glm::mat4& view /*= glm::mat4(1.0f)*/, 
		const glm::mat4& model /*= glm::mat4(1.0f)*/)
	{
		auto program = std::make_shared<sgl::Program>();
		sgl::Shader vertex(sgl::ShaderType::VERTEX_SHADER);
		sgl::Shader fragment(sgl::ShaderType::FRAGMENT_SHADER);
		vertex.LoadFromFile("../Asset/RayMarching.vert");
		fragment.LoadFromFile("../Asset/RayMarching.frag");
		program->AddShader(vertex);
		program->AddShader(fragment);
		program->LinkShader();
		program->Use();
		program->UniformMatrix("projection", projection);
		program->UniformMatrix("view", view);
		program->UniformMatrix("model", model);
		return program;
	}

	std::shared_ptr<sgl::Program> CreateJapaneseFlagProgram(
		const glm::mat4& projection /*= glm::mat4(1.0f)*/, 
		const glm::mat4& view /*= glm::mat4(1.0f)*/, 
		const glm::mat4& model /*= glm::mat4(1.0f)*/)
	{
		auto program = std::make_shared<sgl::Program>();
		sgl::Shader vertex(sgl::ShaderType::VERTEX_SHADER);
		sgl::Shader fragment(sgl::ShaderType::FRAGMENT_SHADER);
		vertex.LoadFromFile("../Asset/JapaneseFlag.vert");
		fragment.LoadFromFile("../Asset/JapaneseFlag.frag");
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
		vertex.LoadFromFile("../Asset/CubeMap.vert");
		fragment.LoadFromFile("../Asset/CubeMap.frag");
		program->AddShader(vertex);
		program->AddShader(fragment);
		program->LinkShader();
		program->Use();
		program->UniformMatrix("projection", projection);
		program->UniformMatrix("view", view);
		program->UniformMatrix("model", model);
		return program;
	}

	std::shared_ptr<sgl::Program> CreateCubeMapHighDynamicRangeProgram(
		const glm::mat4& projection /*= glm::mat4(1.0f)*/, 
		const glm::mat4& view /*= glm::mat4(1.0f)*/, 
		const glm::mat4& model /*= glm::mat4(1.0f)*/)
	{
		auto program = std::make_shared<sgl::Program>();
		Shader vertex(ShaderType::VERTEX_SHADER);
		Shader fragment(ShaderType::FRAGMENT_SHADER);
		vertex.LoadFromFile("../Asset/CubeMapHighDynamicRange.vert");
		fragment.LoadFromFile("../Asset/CubeMapHighDynamicRange.frag");
		program->AddShader(vertex);
		program->AddShader(fragment);
		program->LinkShader();
		program->Use();
		program->UniformMatrix("projection", projection);
		program->UniformMatrix("view", view);
		program->UniformMatrix("model", model);
		return program;
	}

	std::shared_ptr<sgl::Program> CreateEquirectangulareCubeMapProgram(
		const glm::mat4& projection /*= glm::mat4(1.0f)*/, 
		const glm::mat4& view /*= glm::mat4(1.0f)*/, 
		const glm::mat4& model /*= glm::mat4(1.0f)*/)
	{
		auto program = std::make_shared<sgl::Program>();
		Shader vertex(ShaderType::VERTEX_SHADER);
		Shader fragment(ShaderType::FRAGMENT_SHADER);
		vertex.LoadFromFile("../Asset/EquirectangularCubeMap.vert");
		fragment.LoadFromFile("../Asset/EquirectangularCubeMap.frag");
		program->AddShader(vertex);
		program->AddShader(fragment);
		program->LinkShader();
		program->Use();
		program->UniformMatrix("projection", projection);
		program->UniformMatrix("view", view);
		program->UniformMatrix("model", model);
		return program;
	}

	std::shared_ptr<sgl::Program> CreatePhysicallyBasedRenderingProgram(
		const glm::mat4& projection /*= glm::mat4(1.0f)*/,
		const glm::mat4& view /*= glm::mat4(1.0f)*/,
		const glm::mat4& model /*= glm::mat4(1.0f)*/)
	{
		auto program = std::make_shared<sgl::Program>();
		sgl::Shader vertex(sgl::ShaderType::VERTEX_SHADER);
		sgl::Shader fragment(sgl::ShaderType::FRAGMENT_SHADER);
		vertex.LoadFromFile("../Asset/PhysicallyBasedRendering.vert");
		fragment.LoadFromFile("../Asset/PhysicallyBasedRendering.frag");
		program->AddShader(vertex);
		program->AddShader(fragment);
		program->LinkShader();
		program->Use();
		program->UniformMatrix("projection", projection);
		program->UniformMatrix("view", view);
		program->UniformMatrix("model", model);
		return program;
	}

	std::shared_ptr<sgl::Program> CreateIrradianceCubeMapProgram(
		const glm::mat4& projection /*= glm::mat4(1.0f)*/,
		const glm::mat4& view /*= glm::mat4(1.0f)*/,
		const glm::mat4& model /*= glm::mat4(1.0f)*/)
	{
		auto program = std::make_shared<sgl::Program>();
		sgl::Shader vertex(sgl::ShaderType::VERTEX_SHADER);
		sgl::Shader fragment(sgl::ShaderType::FRAGMENT_SHADER);
		vertex.LoadFromFile("../Asset/IrradianceCubeMap.vert");
		fragment.LoadFromFile("../Asset/IrradianceCubeMap.frag");
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
