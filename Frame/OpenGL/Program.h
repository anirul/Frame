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
		void AddInputTextureId(std::uint64_t texture_id) override;
		const std::vector<std::uint64_t>& GetInputTextureIds() const override;
		// Set & get output texture id.
		void AddOutputTextureId(std::uint64_t texture_id) override;
		const std::vector<std::uint64_t>& GetOutputTextureIds() const override;
		// Set the scene to a program.
		void SetSceneTreeId(std::uint64_t scene_id) override;
		std::uint64_t GetSceneTreeId() const override;
		// Remove and check texture id.
		void RemoveTextureId(std::uint64_t texture_id) override;
		bool HasTextureId(std::uint64_t texture_id) const override;
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
		void ThrowIsInTextureIds(std::uint64_t texture_id) const;

	private:
		const Error& error_ = Error::GetInstance();
		const Logger& logger_ = Logger::GetInstance();
		mutable std::map<std::string, int> memoize_map_ = {};
		std::vector<unsigned int> attached_shaders_ = {};
		int program_id_ = 0;
		std::vector<std::uint64_t> input_texture_ids_ = {};
		std::vector<std::uint64_t> output_texture_ids_ = {};
		std::uint64_t scene_tree_id_ = 0;
	};

	// Create a program from a string!
	// Will load a program at location: 
	// - "../Asset/Shader/OpenGL/<name>.vert"
	// - "../Asset/Shader/OpenGL/<name>.frag"
	// Also set the projection view and model to identity.
	std::shared_ptr<ProgramInterface> CreateProgram(const std::string& name);

} // End namespace frame::opengl.
