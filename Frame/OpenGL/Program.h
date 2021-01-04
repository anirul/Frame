#pragma once

#include <optional>
#include <map>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "Frame/Error.h"
#include "Frame/OpenGL/Shader.h"
#include "Frame/ProgramInterface.h"
#include "Frame/Proto/Proto.h"

namespace frame::opengl {

	class Program : public ProgramInterface
	{
	public:
		// Create the program.
		Program();
		// Destructor
		virtual ~Program();

	public:
		// Attach shader to a program.
		void AddShader(const Shader& shader);
		// Link shaders to a program.
		void LinkShader() override;
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
			const glm::vec2 vec2) const override;
		// Create a uniform from a string and a vector3.
		void Uniform(
			const std::string& name,
			const glm::vec3 vec3) const override;
		// Create a uniform from a string and a vector4.
		void Uniform(
			const std::string& name,
			const glm::vec4 vec4) const override;
		// Create a uniform from a string and a matrix.
		void Uniform(
			const std::string& name,
			const glm::mat4 mat) const override;

	protected:
		const int GetMemoizeUniformLocation(const std::string& name) const;

	private:
		const Error& error_ = Error::GetInstance();
		mutable std::map<std::string, int> memoize_map_ = {};
		std::vector<unsigned int> attached_shaders_ = {};
		int program_id_ = 0;
	};

	// Create a program from a string!
	// Will load a program at location: 
	// - "../Asset/<name>.vert"
	// - "../Asset/<name>.frag"
	// Also set the projection view and model to identity.
	std::shared_ptr<ProgramInterface> CreateProgram(const std::string& name);

} // End namespace frame::opengl.
