#pragma once

#include <array>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "Frame/Error.h"
#include "Frame/OpenGL/Buffer.h"
#include "Frame/OpenGL/Material.h"
#include "Frame/OpenGL/Program.h"
#include "Frame/OpenGL/Texture.h"

namespace frame::opengl {

	class StaticMesh : public StaticMeshInterface
	{
	public:
		// Open a mesh from a OBJ stream.
		StaticMesh(
			std::istream& is, 
			const std::string& name);
		// Create a mesh from a set of vectors.
		StaticMesh(
			const std::vector<float>& points,
			const std::vector<float>& normals,
			const std::vector<float>& texcoords,
			const std::vector<std::int32_t>& indices);
		virtual ~StaticMesh();

	public:
		void Draw(const std::shared_ptr<ProgramInterface> program) const;

	public:
		// Set a material for this mesh.
		void SetMaterial(const Material& material) 
		{ 
			material_ = std::make_shared<Material>(material);
		}
		const Buffer& PointBuffer() const { return point_buffer_; }
		const Buffer& NormalBuffer() const { return normal_buffer_; }
		const Buffer& TextureBuffer() const { return texture_buffer_; }
		const Buffer& IndexBuffer() const { return index_buffer_; }
		const size_t IndexSize() const { return index_size_; }
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
		glm::vec2 GetVec2From2Float(
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
		std::shared_ptr<ProgramInterface> program_ = nullptr;
		Buffer point_buffer_ = {};
		Buffer normal_buffer_ = {};
		Buffer texture_buffer_ = {};
		Buffer index_buffer_ =	{ BufferType::ELEMENT_ARRAY_BUFFER };
		std::shared_ptr<MaterialInterface> material_ = nullptr;
		size_t index_size_ = 0;
		unsigned int vertex_array_object_ = 0;
		const Error& error_ = Error::GetInstance();
		std::string material_name_ = "";
	};

	// Create a Quad Mesh that is on the edge of the screen.
	std::shared_ptr<StaticMeshInterface> CreateQuadStaticMesh();

	// Create a Cube Mesh that correspond to a cube map.
	std::shared_ptr<StaticMeshInterface> CreateCubeStaticMesh();

	// Create a new OBJ file from a file.
	std::shared_ptr<StaticMeshInterface> CreateStaticMeshFromObjFile(
		const std::string& file_path);

} // End namespace frame::opengl.
