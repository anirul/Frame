#pragma once

#include <filesystem>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "frame/logger.h"

namespace frame::file {

	/**
	 * @class ObjVertex
	 * @brief Vertex of an obj file, it contain point, normal and texture
	 *        coordinate.
	 */
	struct ObjVertex {
		glm::vec3 point;
		glm::vec3 normal;
		glm::vec2 tex_coord;
	};

	/**
	 * @class ObjMesh
	 * @brief The mesh object that contain the vertex and the materials.
	 */
	class ObjMesh {
	public:
		/**
		 * @brief Constructor create an obj object from a list of vertex,
		 *        indices and materials.
		 * @param points: Vector of vertices.
		 * @param indices: Vector of indices as int.
		 * @param material: Material id (watch out this is an  internal material
		 *        not a entity id type of material!).
		 */
		ObjMesh(
			std::vector<ObjVertex> points,
			std::vector<int> indices,
			int material)
			:
			points_(points),
			indices_(indices),
			material_(material)
		{}
		/**
		 * @brief Will return the list of vertices.
		 * @return Vector of vertices.
		 */
		const std::vector<ObjVertex>& GetVertices() const { return points_; }
		/**
		 * @brief Will return the list of indices.
		 * @return Vector of indices.
		 */
		const std::vector<int>& GetIndices() const { return indices_; }
		/**
		 * @brief Will return an index to the material vector.
		 * @return An index to the material vector.
		 */
		int GetMaterialId() const { return material_; }

	protected:
		std::vector<ObjVertex> points_ = {};
		std::vector<int> indices_ = {};
		int material_ = -1;
	};

	/**
	 * @class ObjMaterial
	 * @brief Material file that old information for material in an obj style.
	 */
	struct ObjMaterial {
		std::string name;
		glm::vec4 ambient_vec4 = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
		std::string ambient_str;
		glm::vec4 diffuse_vec4 = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
		std::string diffuse_str;
		std::string displacement_str;
		float roughness_val = 0.0f;
		std::string roughness_str;
		float metallic_val = 0.0f;
		std::string metallic_str;
		float sheen_val = 0.0f;
		std::string sheen_str;
		std::string emmissive_str;
		std::string normal_str;
	};

	/**
	 * @class Obj
	 * @brief The class that will open an obj file and store data from it on the
	 *        disk.
	 */
	class Obj {
	public:
		/**
		 * @brief Constructor parse from an OBJ file.
		 * @param file_name: File to be open.
		 */
		Obj(const std::filesystem::path& file_name);

	public:
		/**
		 * @brief Get meshes, they are suppose to be sorted by material.
		 * @return The meshes that are in the file.
		 */
		const std::vector<ObjMesh>& GetMeshes() const { return meshes_; }
		/**
		 * @brief Get the materials, id in mesh give the material in the vector
		 *        (*.mtl).
		 * @return The materials that are in the file.
		 */
		const std::vector<ObjMaterial>& GetMaterials() const {
			return materials_;
		}

	protected:
		std::vector<ObjMesh> meshes_ = {};
		std::vector<ObjMaterial> materials_ = {};
		Logger& logger_ = Logger::GetInstance();
	};

}  // End namespace frame::file.
