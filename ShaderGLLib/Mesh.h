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
		Mesh(const std::string& file);
		virtual ~Mesh();

	public:
		const sgl::Buffer& Mesh::PointBuffer() const { return point_buffer_; }
		const sgl::Buffer& Mesh::NormalBuffer() const {	return normal_buffer_; }
		const sgl::Buffer& Mesh::TextureBuffer() const 
		{ 
			return texture_buffer_;
		}
		const sgl::Buffer& Mesh::IndexBuffer() const { return index_buffer_; }
		const size_t IndexSize() const { return index_size_; }
		void SetTextures(std::initializer_list<std::string> values);
		void Draw(
			const sgl::Program& program,
			const sgl::TextureManager& texture_manager,
			const sgl::matrix& model = {}) const;
		
	protected:
		struct ObjFile {
			std::vector<sgl::vector3> positions;
			std::vector<sgl::vector3> normals;
			std::vector<sgl::vector2> textures;
			std::vector<std::array<int, 3>> indices;
		};
		std::optional<ObjFile> LoadFromObj(const std::string& file);

	private:
		std::vector<std::string> textures_ = {};
		sgl::Buffer point_buffer_ = {};
		sgl::Buffer normal_buffer_ = {};
		sgl::Buffer texture_buffer_ = {};
		sgl::Buffer index_buffer_ =	{ sgl::BufferType::ELEMENT_ARRAY_BUFFER };
		size_t index_size_ = 0;
		unsigned int vertex_array_object_ = 0;
	};

} // End namespace sgl.
