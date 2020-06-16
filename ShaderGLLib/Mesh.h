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
#include "../ShaderGLLib/Material.h"

namespace sgl {

	class Mesh
	{
	public:
		// Open a mesh from a OBJ stream.
		Mesh(
			std::istream& is, 
			const std::string& name, 
			const std::shared_ptr<Program>& program);
		// Create a mesh from a set of vectors.
		Mesh(
			const std::vector<float>& points,
			const std::vector<float>& normals,
			const std::vector<float>& texcoords,
			const std::vector<std::int32_t>& indices,
			const std::shared_ptr<Program>& program);
		virtual ~Mesh();

	public:
		// Set a material for this mesh.
		void SetMaterial(const Material& material) { material_ = material; }
		void SetTextures(std::initializer_list<std::string> values);
		void SetTextures(const std::vector<std::string>& vec);
		void Draw(
			const TextureManager& texture_manager,
			const glm::mat4& projection = glm::mat4(1.0f),
			const glm::mat4& view = glm::mat4(1.0f),
			const glm::mat4& model = glm::mat4(1.0f)) const;
		// Set the program for the mesh (it is supposed to be done at creation).
		void SetProgram(const std::shared_ptr<Program>& program);

	public:
		const Buffer& PointBuffer() const { return point_buffer_; }
		const Buffer& NormalBuffer() const { return normal_buffer_; }
		const Buffer& TextureBuffer() const { return texture_buffer_; }
		const Buffer& IndexBuffer() const { return index_buffer_; }
		const size_t IndexSize() const { return index_size_; }
		const std::shared_ptr<Program> GetProgram() { return program_; }
		void ClearDepthBuffer(bool clear) { clear_depth_buffer_ = clear; }
		bool IsClearDepthBuffer() const { return clear_depth_buffer_; }
		const std::string GetMaterialName() const { return material_name_; }

	protected:
		struct ObjFile {
			// Minimum index element this is useful for scene OBJ.
			int min_position = std::numeric_limits<int>::max();
			int min_normal = std::numeric_limits<int>::max();
			int min_texture = std::numeric_limits<int>::max();
			// Position, normal and texture coordinates.
			std::vector<glm::vec3> positions;
			std::vector<glm::vec3> normals;
			std::vector<glm::vec2> textures;
			// Indices you should subtract the min_element.
			std::vector<std::array<int, 3>> indices;
			// List of material to include (should only be one!).
			std::string material = {};
		};
		// Load from OBJ throw an exception in case of error.
		ObjFile LoadFromObj(std::istream& is, const std::string& name);
		// Get a vector from a number of float.
		glm::vec3 GetVec3From3Float(
			std::istream& is,
			const std::string& stream_name,
			const std::string& element_name) const;
		glm::vec2 Getvec2From2Float(
			std::istream& is,
			const std::string& stream_name,
			const std::string& element_name) const;
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
		Material material_ = {};
		size_t index_size_ = 0;
		unsigned int vertex_array_object_ = 0;
		const Error& error_ = Error::GetInstance();
		std::string material_name_ = "";
	};

	// Create a Quad Mesh that is on the edge of the screen.
	std::shared_ptr<Mesh> CreateQuadMesh(
		const std::shared_ptr<Program>& program);

	// Create a Cube Mesh that correspond to a cube map.
	std::shared_ptr<Mesh> CreateCubeMesh(
		const std::shared_ptr<Program>& program);

	// Create a new OBJ file from a file.
	std::shared_ptr<Mesh> CreateMeshFromObjFile(
		const std::string& file_path,
		const std::shared_ptr<Program>& program);

} // End namespace sgl.
