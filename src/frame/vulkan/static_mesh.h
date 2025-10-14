#pragma once

#include "frame/level_interface.h"
#include "frame/static_mesh_interface.h"

namespace frame::vulkan
{

class StaticMesh : public frame::StaticMeshInterface
{
  public:
    StaticMesh(const frame::StaticMeshParameter& parameters, bool clear_buffer);
    ~StaticMesh() override = default;

    EntityId GetPointBufferId() const override
    {
        return parameter_.point_buffer_id;
    }
    EntityId GetColorBufferId() const override
    {
        return parameter_.color_buffer_id;
    }
    EntityId GetNormalBufferId() const override
    {
        return parameter_.normal_buffer_id;
    }
    EntityId GetTextureBufferId() const override
    {
        return parameter_.texture_buffer_id;
    }
    EntityId GetIndexBufferId() const override
    {
        return parameter_.index_buffer_id;
    }
    EntityId GetTriangleBufferId() const override
    {
        return parameter_.triangle_buffer_id;
    }
    EntityId GetBvhBufferId() const override
    {
        return parameter_.bvh_buffer_id;
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
        return clear_buffer_;
    }

  private:
    frame::StaticMeshParameter parameter_ = {};
    std::size_t index_size_ = 0;
    bool clear_buffer_ = true;
};

frame::EntityId CreateQuadStaticMesh(frame::LevelInterface& level);
frame::EntityId CreateCubeStaticMesh(frame::LevelInterface& level);

} // namespace frame::vulkan
