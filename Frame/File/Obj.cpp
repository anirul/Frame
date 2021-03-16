#include "Frame/File/Obj.h"
#include <fstream>
#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include <tiny_obj_loader.h>
#include <fmt/core.h>

namespace frame::file {

	Obj::Obj(const std::string& file_path, const std::string& file_name)
	{
		tinyobj::ObjReaderConfig reader_config;
		reader_config.mtl_search_path = file_path;
		tinyobj::ObjReader reader;
		if (!reader.ParseFromFile(file_path + file_name))
		{
			if (!reader.Error().empty())
			{
				throw std::runtime_error(reader.Error());
			}
			throw std::runtime_error(
				fmt::format(
					"Unknown error parsing file {}/{}",
					file_path,
					file_name));
		}
		
		logger_->info("File {}/{} oppened and parsed.", file_path, file_name);
		if (!reader.Warning().empty())
		{
			logger_->warn(
				"Warning parsing file {}: {}", 
				file_name, 
				reader.Warning());
		}

		auto& attrib = reader.GetAttrib();
		auto& shapes = reader.GetShapes();
		auto& materials = reader.GetMaterials();

		// Loop over shapes
		for (size_t s = 0; s < shapes.size(); s++) 
		{
			std::vector<ObjMesh> meshes;
			// Loop over faces(polygon)
			size_t index_offset = 0;
			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) 
			{
				int fv = shapes[s].mesh.num_face_vertices[f];
				// Loop over vertices in the face.
				for (size_t v = 0; v < fv; v++) {
					// access to vertex
					tinyobj::index_t idx = 
						shapes[s].mesh.indices[index_offset + v];
					tinyobj::real_t vx = 
						attrib.vertices[3 * idx.vertex_index + 0];
					tinyobj::real_t vy = 
						attrib.vertices[3 * idx.vertex_index + 1];
					tinyobj::real_t vz = 
						attrib.vertices[3 * idx.vertex_index + 2];
					tinyobj::real_t nx = 
						attrib.normals[3 * idx.normal_index + 0];
					tinyobj::real_t ny = 
						attrib.normals[3 * idx.normal_index + 1];
					tinyobj::real_t nz = 
						attrib.normals[3 * idx.normal_index + 2];
					tinyobj::real_t tx = 
						attrib.texcoords[2 * idx.texcoord_index + 0];
					tinyobj::real_t ty = 
						attrib.texcoords[2 * idx.texcoord_index + 1];
				}
				index_offset += fv;

				// per-face material
				// shapes[s].mesh.material_ids[f];
			}
		}
	}

} // End namespace frame::file.
