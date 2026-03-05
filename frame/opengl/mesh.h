#pragma once

#include <cstddef>
#include <cstdint>

#include "frame/level_interface.h"
#include "frame/opengl/bind_interface.h"
#include "frame/opengl/buffer.h"
#include "frame/mesh_interface.h"

namespace frame::opengl
{

/**
 * @class Mesh
 * @brief Common OpenGL mesh implementation with VAO/buffer ownership.
 */
class Mesh : public BindInterface, public MeshInterface
{
  public:
    Mesh(LevelInterface& level, const MeshParameter& parameters);
    virtual ~Mesh();

  public:
    EntityId GetPointBufferId() const override
    {
        return point_buffer_id_;
    }
    EntityId GetColorBufferId() const override
    {
        return color_buffer_id_;
    }
    EntityId GetNormalBufferId() const override
    {
        return normal_buffer_id_;
    }
    EntityId GetTextureBufferId() const override
    {
        return texture_buffer_id_;
    }
    EntityId GetIndexBufferId() const override
    {
        return index_buffer_id_;
    }
    EntityId GetTriangleBufferId() const override
    {
        return triangle_buffer_id_;
    }
    EntityId GetBvhBufferId() const override
    {
        return bvh_buffer_id_;
    }
    std::size_t GetIndexSize() const override
    {
        return index_size_;
    }
    void SetIndexSize(std::size_t index_size) override
    {
        index_size_ = index_size;
    }
    bool IsClearBuffer() const override
    {
        return clear_depth_buffer_;
    }

  public:
    void Bind(const unsigned int slot = 0) const override;
    void UnBind() const override;

  public:
    void LockedBind() const override
    {
        locked_bind_ = true;
    }
    void UnlockedBind() const override
    {
        locked_bind_ = false;
    }
    unsigned int GetId() const override
    {
        return vertex_array_object_;
    }

  protected:
    LevelInterface& level_;
    bool clear_depth_buffer_ = true;
    mutable bool locked_bind_ = false;
    EntityId point_buffer_id_ = NullId;
    std::uint32_t point_buffer_size_ = 3;
    EntityId color_buffer_id_ = NullId;
    std::uint32_t color_buffer_size_ = 3;
    EntityId normal_buffer_id_ = NullId;
    std::uint32_t normal_buffer_size_ = 3;
    EntityId texture_buffer_id_ = NullId;
    std::uint32_t texture_buffer_size_ = 2;
    EntityId index_buffer_id_ = NullId;
    EntityId triangle_buffer_id_ = NullId;
    EntityId bvh_buffer_id_ = NullId;
    std::size_t index_size_ = 0;
    unsigned int vertex_array_object_ = 0;
    float point_size_ = 1.0f;
};

/**
 * @brief Create a quad mesh (used for texture effects).
 * @param level: The quad mesh will be added to this level.
 * @return Entity id if successful.
 */
EntityId CreateQuadMesh(LevelInterface& level);

/**
 * @brief Create a cube mesh centered around the origin.
 * @param level: The cube mesh will be added to this level.
 * @return Entity id if successful.
 */
EntityId CreateCubeMesh(LevelInterface& level);

} // End namespace frame::opengl.

