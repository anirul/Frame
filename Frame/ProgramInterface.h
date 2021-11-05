#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "Frame/EntityId.h"
#include "Frame/NameInterface.h"
#include "Frame/Proto/Proto.h"
#include "Frame/UniformInterface.h"

namespace frame {

	struct ProgramInterface : public NameInterface
	{
		// Set & get input texture id.
		virtual void AddInputTextureId(EntityId id) = 0;
		virtual void RemoveInputTextureId(EntityId id) = 0;
		virtual const std::vector<EntityId>	GetInputTextureIds() const = 0;
		// Set & get output texture id.
		virtual void AddOutputTextureId(EntityId id) = 0;
		virtual void RemoveOutputTextureId(EntityId id) = 0;
		virtual const std::vector<EntityId> GetOutputTextureIds() const = 0;
		// Get & Set depth test boolean.
		virtual void SetDepthTest(bool enable) = 0;
		virtual bool GetDepthTest() const = 0;
		// Select the input (temporary) mesh or scene root.
		virtual std::string GetTemporarySceneRoot() const = 0;
		virtual void SetTemporarySceneRoot(const std::string& name) = 0;
		// Select the input mesh or scene root.
		virtual EntityId GetSceneRoot() const = 0;
		virtual void SetSceneRoot(EntityId scene_root) = 0;
		// Link shaders to a program.
		virtual void LinkShader() = 0;
		// Use the program it takes an optional uniform interface pointer to 
		// set the uniform variable parameters.
		virtual void Use(const UniformInterface* uniform_interface) const = 0;
		// Stop using the program.
		virtual void UnUse() const = 0;
		// Get the list of uniforms.
		virtual const std::vector<std::string>& GetUniformNameList() const = 0;
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
		// Add a later included value (like camera position or time), this will
		// be set in the "Use" function.
		virtual void Uniform(
			const std::string& name,
			const proto::Uniform::UniformEnum enum_value) const = 0;
	};

} // End namespace frame.
