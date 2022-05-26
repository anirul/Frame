#include "Frame/File/Obj.h"
#include <fstream>
#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include <tiny_obj_loader.h>
#include <fmt/core.h>
#include "Frame/File/FileSystem.h"

namespace frame::file {

	Obj::Obj(const std::string& file_name)
	{
#ifdef TINY_OBJ_LOADER_V2
		tinyobj::ObjReaderConfig reader_config;
		const auto pair = SplitFileDirectory(file_name);
		reader_config.mtl_search_path = pair.first;
		tinyobj::ObjReader reader;
		std::string total_path = file::FindFile(file_name);
		if (!reader.ParseFromFile(total_path))
		{
			if (!reader.Error().empty())
			{
				throw std::runtime_error(reader.Error());
			}
			throw std::runtime_error(
				fmt::format(
					"Unknown error parsing file [{}].",
					total_path));
		}
		
		logger_->info("Opening OBJ File [{}].", total_path);
		if (!reader.Warning().empty())
		{
			logger_->warn(
				"Warning parsing file {}: {}", 
				total_path, 
				reader.Warning());
		}

		auto& attrib = reader.GetAttrib();
		auto& shapes = reader.GetShapes();
		auto& materials = reader.GetMaterials();
#else
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		const auto pair = SplitFileDirectory(file_name);

		std::string err;
		bool ret = tinyobj::LoadObj(
			&attrib, 
			&shapes, 
			&materials, 
			&err, 
			file::FindFile(file_name).c_str(),
			pair.first.c_str());

		if (!err.empty()) {
			logger_->error(err);
			throw std::runtime_error(err);
		}
#endif // TINY_OBJ_LOADER_V2

		// Loop over shapes
		for (size_t s = 0; s < shapes.size(); s++) 
		{
			std::vector<ObjVertex> points; 
			std::vector<int> indices;
			int material_id = 0;

			// Loop over faces(polygon) this should be triangles?
			size_t index_offset = 0;
			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) 
			{
				// This SHOULD be 3!
				int fv = shapes[s].mesh.num_face_vertices[f];
				if (fv != 3)
				{
					throw std::runtime_error(
						fmt::format(
							"The face should be 3 in size now {}.", 
							fv));
				}
				// Loop over vertices in the face.
				for (size_t v = 0; v < fv; v++) {
					ObjVertex vertex{};
					// access to vertex
					tinyobj::index_t idx = 
						shapes[s].mesh.indices[index_offset + v];
					vertex.point.x = attrib.vertices[3 * idx.vertex_index + 0];
					vertex.point.y = attrib.vertices[3 * idx.vertex_index + 1];
					vertex.point.z = attrib.vertices[3 * idx.vertex_index + 2];
					vertex.normal.x = attrib.normals[3 * idx.normal_index + 0];
					vertex.normal.y = attrib.normals[3 * idx.normal_index + 1];
					vertex.normal.z = attrib.normals[3 * idx.normal_index + 2];
					vertex.tex_coord.x = 
						attrib.texcoords[2 * idx.texcoord_index + 0];
					vertex.tex_coord.y = 
						attrib.texcoords[2 * idx.texcoord_index + 1];
					points.push_back(vertex);
					indices.push_back(static_cast<int>(indices.size()));
				}
				index_offset += fv;

				// per-face material
				if (material_id)
					assert(material_id == shapes[s].mesh.material_ids[f]);
				material_id = shapes[s].mesh.material_ids[f];
			}
			ObjMesh mesh(points, indices, material_id);
			meshes_.push_back(mesh);
		}

		for (const auto& material : materials)
		{
			ObjMaterial obj_material{};
			obj_material.name = material.name;
			obj_material.ambient_str = material.ambient_texname;
			obj_material.ambient_vec4 = glm::vec4(
				material.ambient[0],
				material.ambient[1],
				material.ambient[2],
				material.dissolve);
			obj_material.diffuse_str = material.diffuse_texname;
			obj_material.diffuse_vec4 = glm::vec4(
				material.diffuse[0],
				material.diffuse[1],
				material.diffuse[2],
				material.dissolve);
			obj_material.displacement_str = material.displacement_texname;
			obj_material.emmissive_str = material.emissive_texname;
			obj_material.metallic_str = material.metallic_texname;
			obj_material.metallic_val = material.metallic;
			obj_material.normal_str = material.normal_texname;
			obj_material.roughness_str = material.roughness_texname;
			obj_material.roughness_val = material.roughness;
			obj_material.sheen_str = material.sheen_texname;
			obj_material.sheen_val = material.sheen;
			materials_.emplace_back(obj_material);
		}
	}

} // End namespace frame::file.
