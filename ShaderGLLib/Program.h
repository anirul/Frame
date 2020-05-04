#pragma once

#include <optional>
#include <map>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "../ShaderGLLib/Shader.h"
#include "../ShaderGLLib/Error.h"

namespace sgl {

	class Program {
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
		void Use() const;
		// Create a uniform from a string and a bool.
		void UniformBool(const std::string& name, bool value) const;
		// Create a uniform from a string and an int.
		void UniformInt(const std::string& name, int value) const;
		// Create a uniform from a string and a float.
		void UniformFloat(const std::string& name, float value) const;
		// Create a uniform from a string and a vector2.
		void UniformVector2(
			const std::string& name, 
			const glm::vec2& vec2) const;
		// Create a uniform from a string and a vector3.
		void UniformVector3(
			const std::string& name,
			const glm::vec3& vec3) const;
		// Create a uniform from a string and a vector4.
		void UniformVector4(
			const std::string& name,
			const glm::vec4& vec4) const;
		// Create a uniform from a string and a matrix.
		void UniformMatrix(
			const std::string& name,
			const glm::mat4& mat,
			const bool flip = false) const;

	protected:
		const int GetMemoizeUniformLocation(const std::string& name) const;

	private:
		mutable std::map<std::string, int> memoize_map_ = {};
		std::vector<unsigned int> attached_shaders_ = {};
		int program_id_ = 0;
		const Error& error_ = Error::GetInstance();
	};

	// Create a program from a string!
	// Will load a program at location: 
	// - "../Asset/<name>.vert"
	// - "../Asset/<name>.frag"
	// Also set the projection view and model to identity.
	std::shared_ptr<sgl::Program> CreateProgram(const std::string& name);

} // End namespace sgl.
