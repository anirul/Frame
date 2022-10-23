#pragma once

#include <memory>

#include "frame/node_interface.h"

namespace frame {

/**
 * @class NodeStaticMesh
 * @brief Node for static mesh container. This will hold a static mesh and associated material.
 */
class NodeStaticMesh : public NodeInterface {
   public:
    /**
     * @brief Constructor for node that contain a mesh.
     * @param func: This function return the ID from a string (it will need a level passed in the
     * capture list).
     * @param static_mesh_id: Static mesh to be contained by the node.
     */
    NodeStaticMesh(std::function<NodeInterface*(const std::string&)> func, EntityId static_mesh_id)
        : NodeInterface(func), static_mesh_id_(static_mesh_id) {}
    NodeStaticMesh(std::function<NodeInterface*(const std::string&)> func,
                   const proto::CleanBuffer& clean_buffer)
        : NodeInterface(func), static_mesh_id_(NullId) {
        // Clean the back color and depth if needed.
        for (std::uint32_t flag : clean_buffer.values()) {
            if (flag == proto::CleanBuffer::CLEAR_COLOR)
                clean_buffer_ |= proto::CleanBuffer::CLEAR_COLOR;
            if (flag == proto::CleanBuffer::CLEAR_DEPTH)
                clean_buffer_ |= proto::CleanBuffer::CLEAR_DEPTH;
        }
    }

   public:
    /**
     * @brief Compute the local model of current node.
     * @param dt: Delta time from the beginning of the software running in seconds.
     * @return A mat4 representing the local model matrix.
     */
    glm::mat4 GetLocalModel(const double dt) const override;

   public:
    /**
     * @brief Get local mesh return the local attached mesh.
     * @return Id of the local attached mesh.
     */
    EntityId GetLocalMesh() const override { return static_mesh_id_; }
    /**
     * @brief Get clean buffer parameters.
     * @return Clean buffer.
     */
    std::uint32_t GetCleanBuffer() { return clean_buffer_; }

   private:
    EntityId static_mesh_id_    = NullId;
    std::uint32_t clean_buffer_ = {};
};

}  // End namespace frame.
