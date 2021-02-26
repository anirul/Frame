#include "Program.h"
#include <stdexcept>
#include "Frame/Error.h"
#include "Frame/Logger.h"

namespace frame::opengl {

	std::shared_ptr<ProgramInterface> CreateProgram(
		std::istream& vertex_shader_code,
		std::istream& pixel_shader_code)
	{
		std::string vertex_source(
			std::istreambuf_iterator<char>(vertex_shader_code), 
			{});
		std::string pixel_source(
			std::istreambuf_iterator<char>(pixel_shader_code),
			{});
#ifdef _DEBUG
		auto& logger = Logger::GetInstance();
		logger->info("Creating program");
#endif // _DEBUG
		auto program = std::make_shared<Program>();
		const auto& error = Error::GetInstance();
		Shader vertex(ShaderEnum::VERTEX_SHADER);
		Shader fragment(ShaderEnum::FRAGMENT_SHADER);
		if (!vertex.LoadFromSource(vertex_source))
		{
			error.CreateError(
				vertex.GetErrorMessage(),
				__FILE__,
				__LINE__ - 5);
		}
		if (!fragment.LoadFromSource(pixel_source))
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
		program->Uniform("projection", glm::mat4(1.0));
		program->Uniform("view", glm::mat4(1.0));
		program->Uniform("model", glm::mat4(1.0));
#ifdef _DEBUG
		logger->info("with pointer := {}", static_cast<void*>(program.get()));
#endif // _DEBUG
		return program;
	}

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

	void Program::Uniform(const std::string& name, bool value) const
	{
		glUniform1i(GetMemoizeUniformLocation(name), (int)value);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Program::Uniform(const std::string& name, int value) const
	{
		glUniform1i(GetMemoizeUniformLocation(name), value);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Program::Uniform(const std::string& name, float value) const
	{
		glUniform1f(GetMemoizeUniformLocation(name), value);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Program::Uniform(
		const std::string& name, 
		const glm::vec2 vec2) const
	{
		glUniform2f(GetMemoizeUniformLocation(name), vec2.x, vec2.y);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Program::Uniform(
		const std::string& name, 
		const glm::vec3 vec3) const
	{
		glUniform3f(GetMemoizeUniformLocation(name), vec3.x, vec3.y, vec3.z);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Program::Uniform(
		const std::string& name, 
		const glm::vec4 vec4) const
	{
		glUniform4f(
			GetMemoizeUniformLocation(name),
			vec4.x,
			vec4.y,
			vec4.z,
			vec4.w);
		error_.Display(__FILE__, __LINE__ - 6);
	}

	void Program::Uniform(
		const std::string& name, 
		const glm::mat4 mat) const
	{
		glUniformMatrix4fv(
			GetMemoizeUniformLocation(name),
			1, 
			GL_FALSE,
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

	void Program::AddInputTextureId(EntityId id)
	{
		ThrowIsInTextureIds(id);
		input_texture_ids_.push_back(id);
	}

	void Program::RemoveInputTextureId(EntityId id)
	{
		auto it = std::find(
			input_texture_ids_.begin(), 
			input_texture_ids_.end(), 
			id);
		if (it != input_texture_ids_.end())
		{
			input_texture_ids_.erase(it);
		}
	}

	const std::vector<EntityId> Program::GetInputTextureIds() const
	{
		return input_texture_ids_;
	}

	void Program::AddOutputTextureId(EntityId id)
	{
		ThrowIsInTextureIds(id);
		output_texture_ids_.push_back(id);
	}

	void Program::RemoveOutputTextureId(EntityId id)
	{
		auto it = std::find(
			output_texture_ids_.begin(), 
			output_texture_ids_.end(), 
			id);
		if (it != output_texture_ids_.end())
		{
			output_texture_ids_.erase(it);
		}
	}

	const std::vector<EntityId> Program::GetOutputTextureIds() const
	{
		return output_texture_ids_;
	}

	void Program::AddSceneMeshId(EntityId id)
	{
		scene_mesh_ids_.push_back(id);
	}

	void Program::RemoveSceneMeshId(EntityId id)
	{
		auto it = std::find(scene_mesh_ids_.begin(), scene_mesh_ids_.end(), id);
		if (it != scene_mesh_ids_.end())
		{
			scene_mesh_ids_.erase(it);
		}
	}

	const std::vector<EntityId> Program::GetSceneMeshIds() const
	{
		return scene_mesh_ids_;
	}

	void Program::ThrowIsInTextureIds(EntityId texture_id) const
	{
		if (std::count(
			input_texture_ids_.begin(),
			input_texture_ids_.end(),
			texture_id))
		{
			throw std::runtime_error(
				"Texture: [" + std::to_string(texture_id) +
				" is already in input texture ids.");
		}
		if (std::count(
			output_texture_ids_.begin(),
			output_texture_ids_.end(),
			texture_id))
		{
			throw std::runtime_error(
				"Texture: [" + std::to_string(texture_id) +
				" is already in output texture ids.");
		}
	}

} // End namespace sgl.
