#pragma once

#include <optional>
#include <map>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "../ShaderGLLib/Shader.h"

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
	};

	// Create a simple program (mainly for testing purpose).
	// Vector needed in entry are:
	//		- in_position
	//		- in_normal
	//		- in_texcoord
	// Should also need in uniform texture:
	//		- texture1 (for the albedo).
	std::shared_ptr<sgl::Program> CreateSimpleProgram(
		const glm::mat4& projection = glm::mat4(1.0f), 
		const glm::mat4& view = glm::mat4(1.0f),
		const glm::mat4& model = glm::mat4(1.0f));

	// Create a cube map program to render the environment map.
	// Vector needed in entry are:
	//		- in_position
	// Should also need in uniform texture:
	//		- Skybox (for the color)
	std::shared_ptr<sgl::Program> CreateCubeMapProgram(
		const glm::mat4& projection = glm::mat4(1.0f),
		const glm::mat4& view = glm::mat4(1.0f),
		const glm::mat4& model = glm::mat4(1.0f));

	// Create a PBR program to be used with meshes.
	// Vector needed in entry are:
	//		- in_position
	//		- in_normal
	//		- in_texcoord
	// Should also need in uniform texture:
	//		- Color (for the albedo color)
	//		- Normal (for the normal map)
	//		- Metallic (for the metallic map)
	//		- Roughness (for the roughness map)
	//		- AmbientOcclusion (for the ambient occlusion map)
	// We will also need in vec3 uniform:
	//		- camera_position (the position of the camera)
	//		- light_position[4] (position of the lights)
	//		- light_color[4] (colors of the lights)
	std::shared_ptr<sgl::Program> CreatePBRProgram(
		const glm::mat4& projection = glm::mat4(1.0f),
		const glm::mat4& view = glm::mat4(1.0f),
		const glm::mat4& model = glm::mat4(1.0f));

} // End namespace sgl.
