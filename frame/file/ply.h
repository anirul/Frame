#pragma once

#include <filesystem>
#include <glm/glm.hpp>

#include "frame/logger.h"

namespace frame::file
{

/**
 * @class Ply
 * @brief The class to parse ply object and store them on the disk.
 */
class Ply
{
  public:
    /**
     * @brief Constructor create a Ply parser and parse the file into
     *        buffers.
     * @param file_name: File to be open.
     */
    Ply(std::filesystem::path file_name);
    ~Ply();

  public:
    /**
     * @brief Get the coordinate vector.
     * @return The coordinate vector.
     */
    const std::vector<glm::vec3>& GetVertices() const
    {
        return vertices_;
    }
    /**
     * @brief Get the normal vector (optional).
     * @return The normal vector.
     */
    const std::vector<glm::vec3>& GetNormals() const
    {
        return normals_;
    }
    /**
     * @brief Get color vector (optional).
     * @return The color vector.
     */
    const std::vector<glm::vec3>& GetColors() const
    {
        return colors_;
    }
    /**
     * @brief Get texture coordinates vector (optional).
     * @return The texture coordinates vector.
     */
    const std::vector<glm::vec2>& GetTextureCoordinates() const
    {
        return texture_coordinates_;
    }
    /**
     * @brief Get indices of the meshes (optional).
     * @return The indices of points.
     */
    const std::vector<std::uint32_t>& GetIndices() const
    {
        return indices_;
    }

  protected:
    std::vector<glm::vec3> vertices_ = {};
    std::vector<glm::vec3> normals_ = {};
    std::vector<glm::vec3> colors_ = {};
    std::vector<glm::vec2> texture_coordinates_ = {};
    std::vector<std::uint32_t> indices_ = {};
    Logger& logger_ = Logger::GetInstance();
};

} // End namespace frame::file.
