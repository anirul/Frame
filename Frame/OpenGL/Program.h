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
#include "Frame/UniformInterface.h"

namespace frame::opengl {

	class Program : public ProgramInterface
	{
	public:
		// Create the program.
		Program();
		// Destructor
		virtual ~Program();

	public:
		// Get & set name from the name interface.
		std::string GetName() const override { return name_; }
		void SetName(const std::string& name) override { name_ = name; }

	public:
		// Set & get input texture id.
		void AddInputTextureId(EntityId id) override;
		void RemoveInputTextureId(EntityId id) override;
		const std::vector<EntityId> GetInputTextureIds() const override;
		// Set & get output texture id.
		void AddOutputTextureId(EntityId id) override;
		void RemoveOutputTextureId(EntityId id) override;
		const std::vector<EntityId> GetOutputTextureIds() const override;
		// Get the depth test flag.
		void SetDepthTest(bool enable) final { is_depth_test_ = enable; }
		bool GetDepthTest() const final { return is_depth_test_; }
		// Select temporary input mesh or scene root.
		std::string GetTemporarySceneRoot() const override;
		void SetTemporarySceneRoot(const std::string& name) override;
		// Select the input mesh or scene root.
		EntityId GetSceneRoot() const override;
		void SetSceneRoot(EntityId scene_root) override;
		// Attach shader to a program.
		void AddShader(const Shader& shader);
		// Link shaders to a program.
		void LinkShader() override;
		// Get the list of uniforms needed by the program.
		const std::vector<std::string>& GetUniformNameList() const override;
		// CHECKME(anirul): maybe this can be a Bind and Unbind? then it should
		// CHECKME(anirul): derived from the Scoped bind thing.
		// Use the program.
		void Use(const UniformInterface* uniform_interface) const override;
		// Stop using the program.
		void UnUse() const override;
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
		// Add a later included value (like camera position or time), this will
		// be set in the use function.
		virtual void Uniform(
			const std::string& name,
			const proto::Uniform::UniformEnum enum_value) const override;

	protected:
		const int GetMemoizeUniformLocation(const std::string& name) const;
		void ThrowIsInTextureIds(EntityId texture_id) const;

	private:
		const Error& error_ = Error::GetInstance();
		const Logger& logger_ = Logger::GetInstance();
		mutable std::map<std::string, int> memoize_map_ = {};
		mutable std::map<std::string, proto::Uniform::UniformEnum> 
			uniform_variable_map_ = {};
		mutable std::vector<std::string> uniform_list_ = {};
		std::vector<unsigned int> attached_shaders_ = {};
		std::string temporary_scene_root_ = "";
		std::string name_ = "";
		int program_id_ = 0;
		EntityId scene_root_ = 0;
		bool is_depth_test_ = false;
		std::vector<EntityId> input_texture_ids_ = {};
		std::vector<EntityId> output_texture_ids_ = {};
	};

	// Create a program from two streams:
	// - vertex shader code;
	// - pixel shader code.
	// Also set the matrix for projection/view/model to I.
	std::optional<std::unique_ptr<frame::ProgramInterface>> 
		CreateProgram(
			std::istream& vertex_shader_code,
			std::istream& pixel_shader_code);

} // End namespace frame::opengl.
