#pragma once

#include <memory>

#include "frame/node_interface.h"

namespace frame
{

/**
 * @class NodeStaticMesh
 * @brief Node for static mesh container. This will hold a static mesh and
 *        associated material.
 */
class NodeStaticMesh : public NodeInterface
{
  public:
    /**
     * @brief Constructor for node that contain a mesh.
     * @param func: This function return the ID from a string (it will need
     * a level passed in the capture list).
     * @param static_mesh_id: Static mesh to be contained by the node.
     */
    NodeStaticMesh(
        std::function<NodeInterface*(const std::string&)> func,
        EntityId static_mesh_id)
        : NodeInterface(func), static_mesh_id_(static_mesh_id)
    {
    }
    NodeStaticMesh(
        std::function<NodeInterface*(const std::string&)> func,
        const proto::CleanBuffer& clean_buffer)
        : NodeInterface(func), static_mesh_id_(NullId)
    {
        // Clean the back color and depth if needed.
        for (std::uint32_t flag : clean_buffer.values())
        {
            if (flag == proto::CleanBuffer::CLEAR_COLOR)
                clean_buffer_ |= proto::CleanBuffer::CLEAR_COLOR;
            if (flag == proto::CleanBuffer::CLEAR_DEPTH)
                clean_buffer_ |= proto::CleanBuffer::CLEAR_DEPTH;
        }
    }
    //! @brief Virtual destructor.
    ~NodeStaticMesh() override = default;

  public:
    /**
     * @brief Compute the local model of current node.
     * @param dt: Delta time from the beginning of the software running in
     * seconds.
     * @return A mat4 representing the local model matrix.
     */
    glm::mat4 GetLocalModel(const double dt) const override;

  public:
    /**
     * @brief Return the node type of this node.
     * @return The node type.
     */
    NodeTypeEnum GetNodeType() const override
    {
        return NodeTypeEnum::NODE_STATIC_MESH;
    }
    /**
     * @brief Get the node render time type.
     * @return the render time type.
     */
    proto::SceneStaticMesh::RenderTimeEnum GetRenderTimeType() const
    {
        return render_time_enum_;
    }
    /**
     * @brief Set the node render time type.
     * @param render_time_enum: The render time type.
     */
    void SetRenderTimeType(
        proto::SceneStaticMesh::RenderTimeEnum render_time_enum)
    {
        render_time_enum_ = render_time_enum;
    }
    /**
     * @brief Get local mesh return the local attached mesh.
     * @return Id of the local attached mesh.
     */
    EntityId GetLocalMesh() const override
    {
        return static_mesh_id_;
    }
    /**
     * @brief Get clean buffer parameters.
     * @return Clean buffer.
     */
    std::uint32_t GetCleanBuffer() const
    {
        return clean_buffer_;
    }

  private:
    EntityId static_mesh_id_ = NullId;
    std::uint32_t clean_buffer_ = {};
	proto::SceneStaticMesh::RenderTimeEnum render_time_enum_ =
        proto::SceneStaticMesh::SCENE_RENDER_TIME;
};

} // End namespace frame.
