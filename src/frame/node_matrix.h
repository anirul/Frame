#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Frame/NodeInterface.h"

namespace frame {

/**
 * @class NodeMatrix
 * @brief Node model of a matrix this is the basic step in the scene tree all root should be of
 * matrix type.
 */
class NodeMatrix : public NodeInterface {
   public:
    /**
     * @brief Constructor with a function and matrix mat4 entry.
     * @param func: This function return the ID from a string (it will need a level passed in the
     * capture list).
     * @param matrix: A matrix in mat4 format that represent a transform for this point.
     */
    NodeMatrix(std::function<NodeInterface*(const std::string&)> func, glm::mat4 matrix)
        : NodeInterface(func), matrix_(matrix) {}
    /**
     * @brief Constructor with a function and a quaternion quat entry. This will enable rotation on
     * a perpetual cycle.
     * @param func: This function return the ID from a string (it will need a level passed in the
     * capture list).
     * @param quat: quaternion representing a rotation to this node (note this will be transfered to
     * a mat4 at creation).
     */
    NodeMatrix(std::function<NodeInterface*(const std::string&)> func, glm::quat quat)
        : NodeInterface(func), matrix_(glm::toMat4(quat)), enable_rotation_(true) {}
    /**
     * @brief Constructor with a matrix mat4 entry.
     * @param matrix: A matrix in mat4 format that represent a transform for this point.
     */
    NodeMatrix(glm::mat4 matrix)
        : NodeInterface([](std::string) { return nullptr; }), matrix_(matrix) {}
    /**
     * @brief Constructor with a quaternion quat entry. This will enable rotation on a perpetual
     * cycle.
     * @param quat: quaternion representing a rotation to this node (note this will be transfered to
     * a mat4 at creation).
     */
    NodeMatrix(glm::quat quat)
        : NodeInterface([](std::string) { return nullptr; }),
          matrix_(glm::toMat4(quat)),
          enable_rotation_(true) {}

   public:
    /**
     * @brief Compute the local model of current node.
     * @param dt: Delta time from the beginning of the software running in seconds.
     * @return A mat4 representing the local model matrix.
     */
    glm::mat4 GetLocalModel(const double dt) const override;

   public:
    /**
	* @brief Set local matrix (could be used if you want to move something around).
	* @param matrix: The new matrix.
	*/
    void SetMatrix(glm::mat4 matrix) { matrix_ = matrix; }
    /**
     * @brief Add a flag to the clear flag list.
	 * @param value: Flag(s) to be added (CLEAR_DEPTH/CLEAR_COLOR see proto for list).
     */
    void AddClearFlags(std::uint32_t value) { clear_flags |= value; }
    /**
     * @brief Get clear flags.
	 * @return clear flags (CLEAR_DEPTH/CLEAR_COLOR see proto for list).
     */
    std::uint32_t GetClearFlags() const { return clear_flags; }

   protected:
    /**
     * @brief Compute local rotation at delta time (from the software start).
	 * @param dt: Delta time from software start in second.
     */
    glm::mat4 ComputeLocalRotation(const double dt) const;

   private:
    glm::mat4 matrix_         = glm::mat4(1.f);
    bool enable_rotation_     = false;
    std::uint32_t clear_flags = 0;
};

}  // End namespace frame.
