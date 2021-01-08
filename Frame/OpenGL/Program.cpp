#include "Program.h"
#include <stdexcept>
#include "Frame/Error.h"
#include "Frame/Logger.h"

namespace frame::opengl {

	std::shared_ptr<ProgramInterface> CreateProgram(
		const std::string& name)
	{
#ifdef _DEBUG
		auto& logger = Logger::GetInstance();
		logger->info("Creating program \"{}\"", name);
#endif // _DEBUG
		auto program = std::make_shared<Program>();
		const auto& error = Error::GetInstance();
		Shader vertex(ShaderEnum::VERTEX_SHADER);
		Shader fragment(ShaderEnum::FRAGMENT_SHADER);
		if (!vertex.LoadFromFile("../Asset/Shader/OpenGL/" + name + ".vert"))
		{
			error.CreateError(
				vertex.GetErrorMessage(),
				__FILE__,
				__LINE__ - 5);
		}
		if (!fragment.LoadFromFile("../Asset/Shader/OpenGL/" + name + ".frag"))
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

	void Program::AddInputTextureId(std::uint64_t texture_id)
	{
		ThrowIsInTextureIds(texture_id);
		input_texture_ids_.push_back(texture_id);
	}

	const std::vector<std::uint64_t>& Program::GetInputTextureIds() const
	{
		return input_texture_ids_;
	}

	void Program::AddOutputTextureId(std::uint64_t texture_id)
	{
		ThrowIsInTextureIds(texture_id);
		output_texture_ids_.push_back(texture_id);
	}

	const std::vector<std::uint64_t>& Program::GetOutputTextureIds() const
	{
		return output_texture_ids_;
	}

	void Program::SetSceneTreeId(std::uint64_t scene_id)
	{
		scene_tree_id_ = scene_id;
	}

	std::uint64_t Program::GetSceneTreeId() const
	{
		return scene_tree_id_;
	}

	void Program::RemoveTextureId(std::uint64_t texture_id)
	{
		bool found = false;
		{
			auto it = std::find(
				input_texture_ids_.begin(), 
				input_texture_ids_.end(),
				texture_id);
			if (it != input_texture_ids_.end())
			{
				found = true;
				input_texture_ids_.erase(it);
			}
		}
		{
			auto it = std::find(
				output_texture_ids_.begin(),
				output_texture_ids_.end(),
				texture_id);
			if (it != output_texture_ids_.end())
			{
				found = true;
				output_texture_ids_.erase(it);
			}
		}
		if (!found)
		{
			throw std::runtime_error(
				"Could not find texture: [" + 
				std::to_string(texture_id) + 
				"].");
		}
	}

	bool Program::HasTextureId(std::uint64_t texture_id) const
	{
		if (std::find_if(
			input_texture_ids_.begin(), 
			input_texture_ids_.end(), 
			[&texture_id](auto id) 
			{
				return id == texture_id;
			}) != input_texture_ids_.end())
		{
			return true;
		}
		if (std::find_if(
			output_texture_ids_.begin(),
			output_texture_ids_.end(),
			[&texture_id](auto id)
			{
				return id == texture_id;
			}) != output_texture_ids_.end())
		{
			return true;
		}
		return false;
	}

	void Program::ThrowIsInTextureIds(std::uint64_t texture_id) const
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
