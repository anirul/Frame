#include "frame/file/obj.h"

#include <fstream>
#include <numeric>
#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include <format>
#include <sstream>
#include <tiny_obj_loader.h>

#include "frame/file/file_system.h"

namespace frame::file
{

Obj::Obj(
    const std::filesystem::path& file_name,
    const std::vector<std::filesystem::path>& search_paths)
{
    // First thing first, find the material file.
    std::ifstream obj_file(file_name);
    if (!obj_file.is_open())
    {
        throw std::runtime_error(
            std::format("Could not open file: {}", file_name.string()));
    }
    std::string line;
    std::string mtl_file_name;
    while (std::getline(obj_file, line))
    {
        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;
        if (keyword == "mtllib")
        {
            iss >> mtl_file_name;
            break;
        }
    }

    if (!mtl_file_name.empty())
    {
        auto mtl_path = FindFile(mtl_file_name, search_paths);
        Mtl mtl(mtl_path, search_paths);
        materials_ = mtl.GetMaterials();
    }

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
            std::format("Unknown error parsing file [{}].", total_path));
    }

    logger_->info("Opening OBJ File [{}].", total_path);
    if (!reader.Warning().empty())
    {
        logger_->warn(
            "Warning parsing file {}: {}", total_path, reader.Warning());
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
#else
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    const auto directory = file_name.parent_path();
    const auto file = file_name.filename();

    std::string err;
    std::string warn;
    bool ret = tinyobj::LoadObj(
        &attrib,
        &shapes,
        &materials,
        &warn,
        &err,
        file_name.string().c_str(),
        directory.string().c_str());
    if (!warn.empty())
    {
        logger_->warn("Warning parsing file {}: {}", file_name.string(), warn);
    }
    if (!err.empty())
    {
        logger_->error("Error parsing file {}: {}", file_name.string(), err);
        throw std::runtime_error(err);
    }
#endif // TINY_OBJ_LOADER_V2
    has_texture_coordinates_ = !attrib.texcoords.empty();

    if (shapes.size() == 0)
    {
        std::vector<ObjVertex> points;
        int material_id = 0;
        for (std::vector<float>::iterator ite = attrib.vertices.begin();
             ite != attrib.vertices.end();
             ite += 3)
        {
            ObjVertex obj_vertex;
            obj_vertex.point.x = *ite;
            obj_vertex.point.y = *(ite + 1);
            obj_vertex.point.z = *(ite + 2);
            points.push_back(obj_vertex);
        }
        std::vector<int> indices;
        indices.resize(attrib.vertices.size());
        std::iota(indices.begin(), indices.end(), 1);
        ObjMesh mesh(points, indices, material_id, has_texture_coordinates_);
        meshes_.push_back(mesh);
    }
    else
    {
        // Loop over shapes
        for (size_t s = 0; s < shapes.size(); s++)
        {
            std::vector<ObjVertex> points;
            std::vector<int> indices;
            int material_id = 0;

            // Loop over faces(polygon) this should be triangles?
            size_t index_offset = 0;
            for (std::size_t f = 0; f < shapes[s].mesh.num_face_vertices.size();
                 f++)
            {
                // This SHOULD be 3!
                int fv = shapes[s].mesh.num_face_vertices[f];
                if (fv != 3)
                {
                    throw std::runtime_error(
                        std::format(
                            "The face should be 3 in size now {}.", fv));
                }
                // Loop over vertices in the face.
                for (std::size_t v = 0; v < fv; v++)
                {
                    ObjVertex vertex{};
                    // access to vertex
                    tinyobj::index_t idx =
                        shapes[s].mesh.indices[index_offset + v];
                    vertex.point.x = attrib.vertices[3 * idx.vertex_index + 0];
                    vertex.point.y = attrib.vertices[3 * idx.vertex_index + 1];
                    vertex.point.z = attrib.vertices[3 * idx.vertex_index + 2];
                    if (idx.normal_index >= 0)
                    {
                        vertex.normal.x =
                            attrib.normals[3 * idx.normal_index + 0];
                        vertex.normal.y =
                            attrib.normals[3 * idx.normal_index + 1];
                        vertex.normal.z =
                            attrib.normals[3 * idx.normal_index + 2];
                    }
                    if (idx.texcoord_index >= 0)
                    {
                        vertex.tex_coord.x =
                            attrib.texcoords[2 * idx.texcoord_index + 0];
                        vertex.tex_coord.y =
                            attrib.texcoords[2 * idx.texcoord_index + 1];
                    }
                    points.push_back(vertex);
                    indices.push_back(static_cast<int>(indices.size()));
                }
                index_offset += fv;

                // per-face material
                if (material_id)
                {
                    assert(material_id == shapes[s].mesh.material_ids[f]);
                }
                material_id = shapes[s].mesh.material_ids[f];
            }
            ObjMesh mesh(
                points, indices, material_id, has_texture_coordinates_);
            meshes_.push_back(mesh);
        }
    }
}

Obj::~Obj() = default;

} // End namespace frame::file.
