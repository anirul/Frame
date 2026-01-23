#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#include "frame/camera.h"
#include "frame/node_interface.h"

namespace frame
{

class NodeCamera : public NodeInterface, public Serialize<proto::NodeCamera>
{
  public:
    /**
     * @brief Constructor it will create a camera according to the params.
     * @param func: This function return the ID from a string (it will need
     *        a level passed in the capture list).
     * @param position: Position of the camera.
     * @param target: Position the camera is looking at.
     * @param up: Direction of the up for the camera (normalized).
     * @param fov_degrees: Field of view (angle in degrees of the vertical
     *        field of view).
     * @param aspect_ratio: Aspect ratio of the screen (weight on height).
     * @param near_clip: Near clipping plane (front distance to be drawn).
     * @param far_clip: Far clipping plane (back distance to be drawn).
     */
    NodeCamera(
        std::function<NodeInterface*(const std::string&)> func,
        const glm::vec3 position = glm::vec3{0.f, 0.f, 0.f},
        const glm::vec3 target = glm::vec3{0.f, 0.f, -1.f},
        const glm::vec3 up = glm::vec3{0.f, 1.f, 0.f},
        const float fov_degrees = 65.0f,
        const float aspect_ratio = 16.0f / 9.0f,
        const float near_clip = 0.1f,
        const float far_clip = 1000.0f)
        : NodeInterface(func), camera_(
                                   std::make_unique<Camera>(
                                       position,
                                       glm::normalize(target - position),
                                       up,
                                       fov_degrees,
                                       aspect_ratio,
                                       near_clip,
                                       far_clip))
    {
    }
    //! @brief Virtual destructor.
    virtual ~NodeCamera() override;

  public:
    /**
     * @brief Compute the local model of current node.
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
        return NodeTypeEnum::NODE_CAMERA;
    }

  public:
    /**
     * @brief Get camera associated with this camera node.
     * @return Camera pointer.
     */
    Camera& GetCamera()
    {
        return *camera_.get();
    }
    /**
     * @brief Get camera associated with this camera node (const version).
     * @return Const camera pointer.
     */
    const Camera& GetCamera() const
    {
        return *camera_.get();
    }

  private:
    std::unique_ptr<Camera> camera_ = nullptr;
};

} // End namespace frame.
