#pragma once

#include <memory>

#include "frame/node_interface.h"
#include "frame/serialize.h"

namespace frame
{

/**
 * @class NodeMesh
 * @brief Node for mesh container. This will hold a mesh and
 *        associated material.
 */
class NodeMesh : public NodeInterface,
                       public Serialize<proto::NodeMesh>
{
  public:
    /**
     * @brief Constructor for node that contain a mesh.
     * @param func: This function return the ID from a string (it will need
     * a level passed in the capture list).
     * @param mesh_id: Mesh to be contained by the node.
     */
    NodeMesh(
        std::function<NodeInterface*(const std::string&)> func,
        EntityId mesh_id)
        : NodeInterface(func), mesh_id_(mesh_id)
    {
    }
    NodeMesh(
        std::function<NodeInterface*(const std::string&)> func,
        const proto::CleanBuffer& clean_buffer)
        : NodeInterface(func), mesh_id_(NullId)
    {
        *data_.mutable_clean_buffer() = clean_buffer;
    }
    //! @brief Virtual destructor.
    ~NodeMesh() override;

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
        return NodeTypeEnum::NODE_MESH;
    }
    /**
     * @brief Get local mesh return the local attached mesh.
     * @return Id of the local attached mesh.
     */
    EntityId GetLocalMesh() const override
    {
        return mesh_id_;
    }

  private:
    EntityId mesh_id_ = NullId;
};

} // End namespace frame.


