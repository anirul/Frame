#pragma once

#include <memory>

#include "Frame/LevelInterface.h"
#include "Frame/OpenGL/FrameBuffer.h"
#include "Frame/OpenGL/RenderBuffer.h"
#include "Frame/ProgramInterface.h"
#include "Frame/RendererInterface.h"
#include "Frame/StaticMeshInterface.h"
#include "Frame/UniformInterface.h"

namespace frame::opengl {

/**
 * @class Renderer
 * @brief This is the renderer class this is the class that is doing the rendering part.
 */
class Renderer : public RendererInterface {
   public:
    /**
     * @brief This will also startup the frame and rendering buffer.
     */
    Renderer(LevelInterface* level, std::pair<std::uint32_t, std::uint32_t> size);

   public:
    /**
     * @brief Set the default projection matrix (move elision).
     * @param projection: The projection matrix.
     */
    void SetProjection(glm::mat4 projection) override { projection_ = projection; }
    /**
     * @brief Set the default view matrix (move elision).
     * @param projection: The view matrix.
     */
    void SetView(glm::mat4 view) override { view_ = view; }
    /**
     * @brief Set the default model matrix, can be changed by render mesh (move elision)!
     * @param projection: The model matrix.
     */
    void SetModel(glm::mat4 model) override { model_ = model; }
    /**
     * @brief Set the cubemap target(used in the render mesh method).
     * @param texture_frame: The proto describing the cube map.
     */
    void SetCubeMapTarget(frame::proto::TextureFrame texture_frame) override {
        texture_frame_ = texture_frame;
    }

   public:
    /**
     * @brief Render to a mesh at a dt time.
     * @param static_mesh: Static mesh to render.
     * @param material: Material to be used (or null).
     * @param model_mat: Model matrix to be used.
     * @param dt: Delta time between the beginning of execution and now in seconds.
     */
    void RenderMesh(StaticMeshInterface* static_mesh, MaterialInterface* material = nullptr,
                    glm::mat4 model_mat = glm::mat4(1.0f), double dt = 0.0) override;
    /**
     * @brief Render a node given an id and the id of a material at dt time.
     * @param node_id: Node to be rendered.
     * @param material_id: Material id to be used.
     * @param dt: Delta time between the beginning of execution and now in seconds.
     */
    void RenderNode(EntityId node_id, EntityId material_id = NullId, double dt = 0.0) override;
    /**
     * @brief Render all meshes at a dt time.
     * @param dt: Delta time between the beginning of execution and now in seconds.
     */
    void RenderAllMeshes(double dt = 0.0) override;
    /**
     * @brief Render all children of node id at a dt time.
     * @param node_id: Node to start rendering from.
     * @param dt: Delta time between the beginning of execution and now in seconds.
     */
    void RenderChildren(EntityId node_id, double dt = 0.0) override;
    /**
     * @brief Render all from a root node.
     * @param dt: Delta time between the beginning of execution and now in seconds.
     */
    void RenderFromRootNode(double dt = 0.0) override;
    /**
     * @brief Display to the screen at dt time.
     * @param dt: Delta time between the beginning of execution and now in seconds.
     */
    void Display(double dt = 0.0) override;
    /**
     * @brief Set depth test.
     * @param enable: Enable or disable depth test.
     */
    void SetDepthTest(bool enable) override;

   private:
    // Level shared_ptr.
    LevelInterface* level_ = nullptr;
    Logger& logger_        = Logger::GetInstance();
    // Projection / View / Model matrices.
    glm::mat4 projection_ = glm::mat4(1.0f);
    glm::mat4 view_       = glm::mat4(1.0f);
    glm::mat4 model_      = glm::mat4(1.0f);
    // Frame & Render buffers.
    FrameBuffer frame_buffer_{};
    RenderBuffer render_buffer_{};
    // Display ids.
    EntityId display_program_id_  = 0;
    EntityId display_material_id_ = 0;
    // Texture frame (used in render mesh).
    frame::proto::TextureFrame texture_frame_;
};

}  // End namespace frame::opengl.
