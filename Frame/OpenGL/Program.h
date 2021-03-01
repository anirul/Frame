#pragma once

#include <optional>
#include <map>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "Frame/Error.h"
#include "Frame/Logger.h"
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
		// Set & get input texture id.
		void AddInputTextureId(EntityId id) override;
		void RemoveInputTextureId(EntityId id) override;
		const std::vector<EntityId> GetInputTextureIds() const override;
		// Set & get output texture id.
		void AddOutputTextureId(EntityId id) override;
		void RemoveOutputTextureId(EntityId id) override;
		const std::vector<EntityId> GetOutputTextureIds() const override;
		// Select the input mesh or scene root.
		EntityId GetSceneRoot() const override;
		void SetSceneRoot(EntityId scene_root) override;
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
		void ThrowIsInTextureIds(EntityId texture_id) const;

	private:
		const Error& error_ = Error::GetInstance();
		const Logger& logger_ = Logger::GetInstance();
		mutable std::map<std::string, int> memoize_map_ = {};
		std::vector<unsigned int> attached_shaders_ = {};
		int program_id_ = 0;
		EntityId scene_root_ = 0;
		std::vector<EntityId> input_texture_ids_ = {};
		std::vector<EntityId> output_texture_ids_ = {};
	};

	// Create a program from two streams:
	// - vertex shader code;
	// - pixel shader code.
	// Also set the matrix for projection/view/model to I.
	std::shared_ptr<frame::ProgramInterface> CreateProgram(
		std::istream& vertex_shader_code,
		std::istream& pixel_shader_code);

} // End namespace frame::opengl.
