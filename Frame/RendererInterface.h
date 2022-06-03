#pragma once

#include <glm/glm.hpp>

#include "Frame/MaterialInterface.h"
#include "Frame/StaticMeshInterface.h"

namespace frame {

/**
 * @class RendererInterface
 * @brief This is the renderer class this is the class that is doing the rendering part.
 */
struct RendererInterface {
    //! Virtual destructor.
    virtual ~RendererInterface() = default;
    /**
     * @brief Set the default projection matrix (move elision).
     * @param projection: The projection matrix.
     */
    virtual void SetProjection(glm::mat4 projection) = 0;
    /**
     * @brief Set the default view matrix (move elision).
     * @param projection: The view matrix.
     */
    virtual void SetView(glm::mat4 view) = 0;
    /**
     * @brief Set the default model matrix, can be changed by render mesh (move elision)!
     * @param projection: The model matrix.
     */
    virtual void SetModel(glm::mat4 model) = 0;
    /**
     * @brief Set the cubemap target(used in the render mesh method).
     * @param texture_frame: The proto describing the cube map.
     */
    virtual void SetCubeMapTarget(frame::proto::TextureFrame texture_frame) = 0;
    /**
     * @brief Render to a mesh at a dt time.
     * @param static_mesh: Static mesh to render.
     * @param material: Material to be used (or null).
     * @param model_mat: Model matrix to be used.
     * @param dt: Delta time between the beginning of execution and now in seconds.
     */
    virtual void RenderMesh(StaticMeshInterface* static_mesh, MaterialInterface* material = nullptr,
                            glm::mat4 model_mat = glm::mat4(1.0f), double dt = 0.0) = 0;
    /**
     * @brief Render a node given an id and the id of a material at dt time.
     * @param node_id: Node to be rendered.
     * @param material_id: Material id to be used.
     * @param dt: Delta time between the beginning of execution and now in seconds.
     */
    virtual void RenderNode(EntityId node_id, EntityId material_id = NullId, double dt = 0.0) = 0;
    /**
     * @brief Render all meshes at a dt time.
     * @param dt: Delta time between the beginning of execution and now in seconds.
     */
    virtual void RenderAllMeshes(double dt = 0.0) = 0;
    /**
     * @brief Render all children of node id at a dt time.
     * @param node_id: Node to start rendering from.
     * @param dt: Delta time between the beginning of execution and now in seconds.
     */
    virtual void RenderChildren(EntityId node_id, double dt = 0.0) = 0;
    /**
     * @brief Render all from a root node.
     * @param dt: Delta time between the beginning of execution and now in seconds.
     */
    virtual void RenderFromRootNode(double dt = 0.0) = 0;
    /**
     * @brief Display to the screen at dt time.
     * @param dt: Delta time between the beginning of execution and now in seconds.
     */
    virtual void Display(double dt = 0.0) = 0;
    /**
     * @brief Set depth test.
     * @param enable: Enable or disable depth test.
     */
    virtual void SetDepthTest(bool enable) = 0;
};

}  // End namespace frame.
