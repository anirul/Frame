#pragma once

#include <optional>
#include <map>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "../ShaderGLLib/Shader.h"
#include "../ShaderGLLib/Error.h"

namespace sgl {

	struct ProgramInterface
	{
		virtual ~ProgramInterface() = default;
		// Use the program.
		virtual void Use() const = 0;
		// Create a uniform from a string and a bool.
		virtual void Uniform(const std::string& name, bool value) const = 0;
		// Create a uniform from a string and an int.
		virtual void Uniform(const std::string& name, int value) const = 0;
		// Create a uniform from a string and a float.
		virtual void Uniform(const std::string& name, float value) const = 0;
		// Create a uniform from a string and a vector2.
		virtual void Uniform(
			const std::string& name,
			const glm::vec2& vec2) const = 0;
		// Create a uniform from a string and a vector3.
		virtual void Uniform(
			const std::string& name,
			const glm::vec3& vec3) const = 0;
		// Create a uniform from a string and a vector4.
		virtual void Uniform(
			const std::string& name,
			const glm::vec4& vec4) const = 0;
		// Create a uniform from a string and a matrix.
		virtual void Uniform(
			const std::string& name,
			const glm::mat4& mat,
			const bool flip = false) const = 0;
	};

	class Program : public ProgramInterface
	{
	public:
		// Create a program from a string!
		// Will load a program at location: 
		// - "../Asset/<name>.vert"
		// - "../Asset/<name>.frag"
		// Also set the projection view and model to identity.
		static std::shared_ptr<sgl::Program> CreateProgram(
			const std::string& name);

	public:
		// Create the program.
		Program();
		// Destructor
		virtual ~Program();

	public:
		// Attach shader to a program.
		void AddShader(const Shader& shader);
		// Link shaders to a program.
		void LinkShader();
		// Use the program.
		void Use() const override;
		// Create a uniform from a string and a bool.
		void Uniform(const std::string& name, bool value) const override;
		// Create a uniform from a string and an int.
		void Uniform(const std::string& name, int value) const override;
		// Create a uniform from a string and a float.
		void Uniform(const std::string& name, float value) const override;
		// Create a uniform from a string and a vector2.
		void Uniform(
			const std::string& name, 
			const glm::vec2& vec2) const override;
		// Create a uniform from a string and a vector3.
		void Uniform(
			const std::string& name,
			const glm::vec3& vec3) const override;
		// Create a uniform from a string and a vector4.
		void Uniform(
			const std::string& name,
			const glm::vec4& vec4) const override;
		// Create a uniform from a string and a matrix.
		void Uniform(
			const std::string& name,
			const glm::mat4& mat,
			const bool flip = false) const override;

	protected:
		const int GetMemoizeUniformLocation(const std::string& name) const;

	private:
		const Error& error_ = Error::GetInstance();
		mutable std::map<std::string, int> memoize_map_ = {};
		std::vector<unsigned int> attached_shaders_ = {};
		int program_id_ = 0;
	};

} // End namespace sgl.
