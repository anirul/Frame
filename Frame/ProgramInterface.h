#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "Frame/EntityId.h"

namespace frame {

	struct ProgramInterface
	{
		// Set & get input texture id.
		virtual void AddInputTextureId(EntityId id) = 0;
		virtual void RemoveInputTextureId(EntityId id) = 0;
		virtual const std::vector<EntityId>	GetInputTextureIds() const = 0;
		// Set & get output texture id.
		virtual void AddOutputTextureId(EntityId id) = 0;
		virtual void RemoveOutputTextureId(EntityId id) = 0;
		virtual const std::vector<EntityId> GetOutputTextureIds() const = 0;
		// Set scene meshes to a program.
		virtual void AddSceneMeshId(EntityId id) = 0;
		virtual void RemoveSceneMeshId(EntityId id) = 0;
		virtual const std::vector<EntityId> GetSceneMeshIds() const = 0;
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
