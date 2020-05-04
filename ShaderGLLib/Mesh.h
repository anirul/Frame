#pragma once

#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <array>
#include <initializer_list>
#include "../ShaderGLLib/Buffer.h"
#include "../ShaderGLLib/Program.h"
#include "../ShaderGLLib/Texture.h"
#include "../ShaderGLLib/Error.h"

namespace sgl {

	class Mesh
	{
	public:
		Mesh(const std::string& file, const std::shared_ptr<Program>& program);
		Mesh(
			const std::vector<float>& points,
			const std::vector<float>& normals,
			const std::vector<float>& texcoords,
			const std::vector<std::int32_t>& indices,
			const std::shared_ptr<Program>& program);
		virtual ~Mesh();

	public:
		void SetTextures(std::initializer_list<std::string> values);
		void SetTextures(const std::vector<std::string>& vec);
		void Draw(
			const TextureManager& texture_manager,
			const glm::mat4& projection = glm::mat4(1.0f),
			const glm::mat4& view = glm::mat4(1.0f),
			const glm::mat4& model = glm::mat4(1.0f)) const;

	public:
		const Buffer& PointBuffer() const { return point_buffer_; }
		const Buffer& NormalBuffer() const {	return normal_buffer_; }
		const Buffer& TextureBuffer() const 
		{ 
			return texture_buffer_;
		}
		const Buffer& IndexBuffer() const { return index_buffer_; }
		const size_t IndexSize() const { return index_size_; }
		const std::shared_ptr<Program> GetProgram() { return program_; }
		void ClearDepthBuffer(bool clear) { clear_depth_buffer_ = clear; }
		
	protected:
		struct ObjFile {
			std::vector<glm::vec3> positions;
			std::vector<glm::vec3> normals;
			std::vector<glm::vec2> textures;
			std::vector<std::array<int, 3>> indices;
		};
		// Load from OBJ throw an exception in case of error.
		ObjFile LoadFromObj(const std::string& file);
		void SetProgram(const std::shared_ptr<Program>& program);
		void CreateMeshFromFlat(
			const std::vector<float>& points,
			const std::vector<float>& normals,
			const std::vector<float>& texcoords,
			const std::vector<std::int32_t>& indices);

	protected:
		bool clear_depth_buffer_ = false;
		std::shared_ptr<Program> program_ = nullptr;
		std::vector<std::string> textures_ = {};
		Buffer point_buffer_ = {};
		Buffer normal_buffer_ = {};
		Buffer texture_buffer_ = {};
		Buffer index_buffer_ =	{ sgl::BufferType::ELEMENT_ARRAY_BUFFER };
		size_t index_size_ = 0;
		unsigned int vertex_array_object_ = 0;
		const Error& error_ = Error::GetInstance();
	};

	// Create a Quad Mesh that is on the edge of the screen.
	std::shared_ptr<Mesh> CreateQuadMesh(
		const std::shared_ptr<Program>& program);

	// Create a Cube Mesh that correspond to a cube map.
	std::shared_ptr<Mesh> CreateCubeMesh(
		const std::shared_ptr<Program>& program);

} // End namespace sgl.
