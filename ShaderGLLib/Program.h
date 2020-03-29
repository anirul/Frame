#pragma once

#include <optional>
#include <map>
#include "../ShaderGLLib/Shader.h"
#include "../ShaderGLLib/Vector.h"

namespace sgl {

	class Program {
	public:
		// Create the program.
		Program();
		// Destructor
		virtual ~Program();
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
			const sgl::vector2& vec2) const;
		// Create a uniform from a string and a vector3.
		void UniformVector3(
			const std::string& name,
			const sgl::vector3& vec3) const;
		// Create a uniform from a string and a vector4.
		void UniformVector4(
			const std::string& name,
			const sgl::vector4& vec4) const;
		// Create a uniform from a string and a matrix.
		void UniformMatrix(
			const std::string& name,
			const sgl::matrix& mat,
			const bool flip = false) const;

	protected:
		const int GetMemoizeUniformLocation(const std::string& name) const;

	private:
		mutable std::map<std::string, int> memoize_map_ = {};
		std::vector<unsigned int> attached_shaders_ = {};
		int program_id_ = 0;
	};

} // End namespace sgl.
