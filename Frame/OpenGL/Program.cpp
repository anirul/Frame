#include "Program.h"
#include <stdexcept>
#include "Frame/Error.h"
#include "Frame/Logger.h"

namespace frame::opengl {

	std::optional<std::unique_ptr<ProgramInterface>> 
		CreateProgram(
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
		auto program = std::make_unique<Program>();
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
		program->Use(nullptr);
		program->Uniform("projection", glm::mat4(1.0));
		program->Uniform("view", glm::mat4(1.0));
		program->Uniform("model", glm::mat4(1.0));
		program->UnUse();
#ifdef _DEBUG
		logger->info("with pointer := {}", static_cast<void*>(program.get()));
#endif // _DEBUG
		return std::move(program);
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

	void Program::Use(const UniformInterface* uniform_interface) const
	{
		glUseProgram(program_id_);
		error_.Display(__FILE__, __LINE__ - 1);
		// Now loop into the uniform map to include uniform interface values.
		for (const auto& pair : uniform_variable_map_)
		{
			switch (pair.second)
			{
				case proto::Uniform::PROJECTION_MAT4 :
				{
					Uniform(pair.first, uniform_interface->GetProjection());
					break;
				}
				case proto::Uniform::PROJECTION_INV_MAT4 :
				{
					Uniform(
						pair.first, 
						glm::inverse(uniform_interface->GetProjection()));
					break;
				}
				case proto::Uniform::VIEW_MAT4 :
				{
					Uniform(pair.first, uniform_interface->GetView());
					break;
				}
				case proto::Uniform::VIEW_INV_MAT4 :
				{
					Uniform(
						pair.first, 
						glm::inverse(uniform_interface->GetView()));
					break;
				}
				case proto::Uniform::MODEL_MAT4 :
				{
					Uniform(pair.first, uniform_interface->GetModel());
					break;
				}
				case proto::Uniform::MODEL_INV_MAT4 :
				{
					Uniform(
						pair.first, 
						glm::inverse(uniform_interface->GetModel()));
					break;
				}
				case proto::Uniform::CAMERA_POSITION_VEC3 :
				{
					Uniform(pair.first, uniform_interface->GetCameraPosition());
					break;
				}
				case proto::Uniform::CAMERA_DIRECTION_VEC3 :
				{
					Uniform(
						pair.first, 
						uniform_interface->GetCameraFront() - 
							uniform_interface->GetCameraPosition());
					break;
				}
				case proto::Uniform::FLOAT_TIME_S :
				{
					Uniform(
						pair.first,
						static_cast<float>(uniform_interface->GetDeltaTime()));
					break;
				}
				case proto::Uniform::INVALID:
				default:
					throw std::runtime_error(
						fmt::format("Unknown enum value {}", pair.second));
			}
		}
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

	void Program::Uniform(
		const std::string& name, 
		const proto::Uniform::UniformEnum enum_value) const
	{
		uniform_variable_map_.insert({ name, enum_value });
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
				"] is already in output texture ids.");
		}
	}

	EntityId Program::GetSceneRoot() const
	{
		// TODO(anirul): Change me with an assert.
		if (!scene_root_) throw std::runtime_error("Should not be null!");
		return scene_root_;
	}

	void Program::SetSceneRoot(EntityId scene_root)
	{
		scene_root_ = scene_root;
	}

	void Program::UnUse() const
	{
		glUseProgram(0);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	const std::vector<std::string>& Program::GetUniformNameList() const
	{
		uniform_list_.clear();
		GLint count = 0;
		glGetProgramiv(program_id_, GL_ACTIVE_UNIFORMS, &count);
		error_.Display(__FILE__, __LINE__ - 1);
		for (GLuint i = 0; i < static_cast<GLuint>(count); ++i)
		{
			constexpr GLsizei max_size = 256;
			GLsizei length = 0;
			GLsizei size = 0;
			GLenum type = 0;
			GLchar name[max_size];
			glGetActiveUniform(
				program_id_, 
				i, 
				max_size, 
				&length, 
				&size, 
				&type, 
				name);
			error_.Display(__FILE__, __LINE__ - 8);
			std::string name_str = std::string(name, name + length);
			uniform_list_.push_back(name_str);
			std::string warning_str = fmt::format(
				"Program[{}].[{}]<{}>({})", 
				program_id_, 
				name_str, 
				type, 
				size);
			logger_->info(warning_str);
		}
		return uniform_list_;
	}

	std::string Program::GetTemporarySceneRoot() const
	{
		return temporary_scene_root_;
	}

	void Program::SetTemporarySceneRoot(const std::string& name)
	{
		temporary_scene_root_ = name;
	}

} // End namespace frame::opengl.
