#pragma once

#include <filesystem>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "frame/file/mtl.h"
#include "frame/logger.h"

namespace frame::file
{

/**
 * @class ObjVertex
 * @brief Vertex of an obj file, it contain point, normal and texture
 *        coordinate.
 */
struct ObjVertex
{
    glm::vec3 point;
    glm::vec3 normal;
    glm::vec2 tex_coord;
};

/**
 * @class ObjMesh
 * @brief The mesh object that contain the vertex and the materials.
 */
class ObjMesh
{
  public:
    /**
     * @brief Constructor create an obj object from a list of vertex,
     *        indices and materials.
     * @param points: Vector of vertices.
     * @param indices: Vector of indices as int.
     * @param material: Material id (watch out this is an  internal material
     *        not a entity id type of material!).
     * @param has_texture_coordinates: Does the mesh has texture coordinates?
     */
    ObjMesh(
        std::vector<ObjVertex> points,
        std::vector<int> indices,
        int material,
        bool has_texture_coordinates)
        : points_(points),
          indices_(indices),
          material_(material),
          has_texture_coordinates_(has_texture_coordinates)
    {
    }
    /**
     * @brief Will return the list of vertices.
     * @return Vector of vertices.
     */
    const std::vector<ObjVertex>& GetVertices() const
    {
        return points_;
    }
    /**
     * @brief Will return the list of indices.
     * @return Vector of indices.
     */
    const std::vector<int>& GetIndices() const
    {
        return indices_;
    }
    /**
     * @brief Will return an index to the material vector.
     * @return An index to the material vector.
     */
    int GetMaterialId() const
    {
        return material_;
    }
    /**
     * @brief Return true if the mesh has texture coordinates.
     * @return True if the mesh has texture coordinates.
     */
    bool HasTextureCoordinates() const
    {
        return has_texture_coordinates_;
    }

  protected:
    std::vector<ObjVertex> points_ = {};
    std::vector<int> indices_ = {};
    int material_ = -1;
    bool has_texture_coordinates_ = false;
};

/**
 * @class Obj
 * @brief The class that will open an obj file and store data from it on the
 *        disk.
 */
class Obj
{
  public:
    /**
     * @brief Constructor parse from an OBJ file.
     * @param file_name: File to be open.
     * @param search_paths: Paths to search for MTL files in.
     */
    Obj(const std::filesystem::path& file_name,
        const std::vector<std::filesystem::path>& search_paths = {});
    ~Obj();

  public:
    /**
     * @brief Get meshes, they are suppose to be sorted by material.
     * @return The meshes that are in the file.
     */
    const std::vector<ObjMesh>& GetMeshes() const
    {
        return meshes_;
    }
    /**
     * @brief Get the materials, id in mesh give the material in the vector
     *        (*.mtl).
     * @return The materials that are in the file.
     */
    const std::vector<MtlMaterial>& GetMaterials() const
    {
        return materials_;
    }
    /**
     * @brief Return true if the OBJ has texture coordinates.
     * @return True if the OBJ has texture coordinates.
     */
    bool HasTextureCoordinates() const
    {
        return has_texture_coordinates_;
    }

  protected:
    std::vector<ObjMesh> meshes_ = {};
    std::vector<MtlMaterial> materials_ = {};
    bool has_texture_coordinates_ = false;
    Logger& logger_ = Logger::GetInstance();
};

} // End namespace frame::file.
