#pragma once

#include <string>
#include <glm/glm.hpp>

namespace frame {

	struct ProgramInterface
	{
		// Link shaders to a program.
		virtual void LinkShader() = 0;
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
			const glm::vec2 vec2) const = 0;
		// Create a uniform from a string and a vector3.
		virtual void Uniform(
			const std::string& name,
			const glm::vec3 vec3) const = 0;
		// Create a uniform from a string and a vector4.
		virtual void Uniform(
			const std::string& name,
			const glm::vec4 vec4) const = 0;
		// Create a uniform from a string and a matrix.
		virtual void Uniform(
			const std::string& name,
			const glm::mat4 mat) const = 0;
	};

} // End namespace frame.
