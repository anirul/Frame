#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "frame/node_interface.h"
#include "frame/serialize.h"

namespace frame
{

/**
 * @class NodeMatrix
 * @brief Node model of a matrix this is the basic step in the scene tree
 * all root should be of matrix type.
 */
class NodeMatrix : public NodeInterface, public Serialize<proto::NodeMatrix>
{
  public:
    /**
     * @brief Constructor with a function and matrix mat4 entry.
     * @param func: This function return the ID from a string (it will need
     * a level passed in the capture list).
     * @param matrix: A matrix in mat4 format that represent a transform for
     * this point.
     */
    NodeMatrix(
        std::function<NodeInterface*(const std::string&)> func,
        glm::mat4 matrix,
        bool rotation = false);
    /**
     * @brief Constructor with a function and a quaternion quat entry. This
     * will enable rotation on a perpetual cycle.
     * @param func: This function return the ID from a string (it will need
     * a level passed in the capture list).
     * @param quat: quaternion representing a rotation to this node (note
     * this will be transfered to a mat4 at creation).
     */
    NodeMatrix(
        std::function<NodeInterface*(const std::string&)> func,
        glm::vec4 quat,
        bool rotation = false);
    /**
     * @brief Constructor with a matrix mat4 entry.
     * @param matrix: A matrix in mat4 format that represent a transform for
     * this point.
     */
    NodeMatrix(glm::mat4 matrix, bool rotation = false);
    /**
     * @brief Constructor with a quaternion quat entry. This will enable
     * rotation on a perpetual cycle.
     * @param quat: quaternion representing a rotation to this node (note
     * this will be transfered to a mat4 at creation).
     */
    NodeMatrix(glm::vec4 quat, bool rotation = false);
    //! @brief Virtual destructor.
    ~NodeMatrix() override = default;

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
        return NodeTypeEnum::NODE_MATRIX;
    }

  protected:
    /**
     * @brief Compute local rotation at delta time (from the software
     * start).
     * @param dt: Delta time from software start in second.
     */
    glm::mat4 ComputeLocalRotation(const double dt) const;
};

} // End namespace frame.
