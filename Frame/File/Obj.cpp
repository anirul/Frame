#include "Frame/File/Obj.h"
#include <fstream>
#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include <tiny_obj_loader.h>
#include <fmt/core.h>
#include "Frame/File/FileSystem.h"

namespace frame::file {

	Obj::Obj(const std::string& file_name)
	{
		tinyobj::ObjReaderConfig reader_config;
		// TODO(anirul): Fix me!
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

		// Loop over shapes
		for (size_t s = 0; s < shapes.size(); s++) 
		{
			std::vector<ObjVertex> points; 
			std::vector<int> indices;
			int material = 0;

			// Loop over faces(polygon) this should be triangles?
			size_t index_offset = 0;
			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) 
			{
				int fv = shapes[s].mesh.num_face_vertices[f];
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
				}
				index_offset += fv;

				// per-face material
				if (material)
					assert(material == shapes[s].mesh.material_ids[f]);
				material = shapes[s].mesh.material_ids[f];
			}
			ObjMesh mesh(points, indices, material);
			meshes_.push_back(mesh);
		}
	}

} // End namespace frame::file.
