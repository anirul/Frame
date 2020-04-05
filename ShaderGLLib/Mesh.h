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

namespace sgl {

	class Mesh
	{
	public:
		Mesh(const std::string& file, const std::shared_ptr<Program>& program);
		virtual ~Mesh();

	public:
		void SetTextures(std::initializer_list<std::string> values);
		void Draw(
			const TextureManager& texture_manager,
			const glm::mat4& model = glm::mat4(1.0f)) const;

	public:
		const sgl::Buffer& Mesh::PointBuffer() const { return point_buffer_; }
		const sgl::Buffer& Mesh::NormalBuffer() const {	return normal_buffer_; }
		const sgl::Buffer& Mesh::TextureBuffer() const 
		{ 
			return texture_buffer_;
		}
		const sgl::Buffer& Mesh::IndexBuffer() const { return index_buffer_; }
		const size_t IndexSize() const { return index_size_; }
		std::shared_ptr<Program> GetProgram() { return program_; }
		void ClearDepthBuffer(bool clear) { clear_depth_buffer_ = clear; }
		
	protected:
		struct ObjFile {
			std::vector<glm::vec3> positions;
			std::vector<glm::vec3> normals;
			std::vector<glm::vec2> textures;
			std::vector<std::array<int, 3>> indices;
		};
		std::optional<ObjFile> LoadFromObj(const std::string& file);
		void SetProgram(const std::shared_ptr<Program>& program);

	protected:
		bool clear_depth_buffer_ = false;
		std::shared_ptr<Program> program_ = nullptr;
		std::vector<std::string> textures_ = {};
		sgl::Buffer point_buffer_ = {};
		sgl::Buffer normal_buffer_ = {};
		sgl::Buffer texture_buffer_ = {};
		sgl::Buffer index_buffer_ =	{ sgl::BufferType::ELEMENT_ARRAY_BUFFER };
		size_t index_size_ = 0;
		unsigned int vertex_array_object_ = 0;
	};

} // End namespace sgl.
