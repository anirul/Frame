#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "Frame/Logger.h"

namespace frame::file {

	struct ObjVertex 
	{
		glm::vec3 point;
		glm::vec3 normal;
		glm::vec2 tex_coord;
	};

	class ObjMesh 
	{
	public:
		ObjMesh(
			std::vector<ObjVertex> points, 
			std::vector<int> indices, 
			int material) : 
			points_(points), indices_(indices), material_(material) {}
		const std::vector<ObjVertex>& GetVertices() const { return points_; }
		const std::vector<int>& GetIndices() const { return indices_; }
		int GetMaterialId() const { return material_; }

	protected:
		std::vector<ObjVertex> points_ = {};
		std::vector<int> indices_ = {};
		int material_ = -1;
	};

	struct ObjMaterial
	{
		glm::vec4 ambient_vec4 = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
		std::string ambient_str = "";
		glm::vec4 diffuse_vec4 = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
		std::string diffuse_str = "";
		std::string displacement_str = "";
		float roughness_val = 0.0f;
		std::string roughness_str = "";
		float metallic_val = 0.0f;
		std::string metallic_str = "";
		float sheen_val = 0.0f;
		std::string sheen_str = "";
		std::string emmissive_str = "";
		std::string normal_str = "";
	};

	class Obj 
	{
	public:
		// Parse from an OBJ file.
		Obj(const std::string& file_name);
		// Get meshes, they are suppose to be sorted by material.
		const std::vector<ObjMesh> GetMeshes() const { return meshes_; }
		// Get the materials, id in mesh give the material in the vector.
		const std::vector<ObjMaterial> GetMaterials() const 
		{ 
			return materials_; 
		}
	
	protected:
		std::vector<ObjMesh> meshes_;
		std::vector<ObjMaterial> materials_;
		Logger& logger_ = Logger::GetInstance();
	};

} // End namespace frame::file.
