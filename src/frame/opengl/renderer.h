#pragma once

#include <memory>

#include "frame/opengl/frame_buffer.h"
#include "frame/opengl/render_buffer.h"
#include "frame/program_interface.h"
#include "frame/renderer_interface.h"
#include "frame/static_mesh_interface.h"
#include "frame/uniform_interface.h"
#include "frame/window_interface.h"

namespace frame::opengl {

/**
 * @class Renderer
 * @brief This is the renderer class this is the class that is doing the rendering part.
 */
class Renderer : public RendererInterface {
   public:
    /**
     * @brief Constructor
     * @param level: The level to render.
     * @param viewport: The viewport.
     */
    Renderer(LevelInterface& level, glm::uvec4 viewport);

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
    /**
     * @brief Get the last program id.
     * @return The id of the last program used by this renderer.
     */
    EntityId GetLastProgramId() const { return last_program_id_; }
    /**
     * @brief Set the current viewport.
     * @param viewport: New viewport.
     */
    void SetViewport(glm::uvec4 viewport) override { viewport_ = viewport; }
    /**
     * @brief Add a mesh render callback.
     * @param callback: The callback to be added to the render.
     */
    void SetMeshRenderCallback(RenderCallback callback) override { callback_ = callback; }

   public:
    /**
     * @brief Render to a mesh at a dt time.
     * @param static_mesh: Static mesh to render.
     * @param material: Material to be used (or null).
     * @param model_mat: Model matrix to be used.
     * @param dt: Delta time between the beginning of execution and now in seconds.
     */
    void RenderMesh(StaticMeshInterface& static_mesh, MaterialInterface& material,
                    const glm::mat4& projection, const glm::mat4& view = glm::mat4(1.0f),
                    const glm::mat4& model = glm::mat4(1.0f), double dt = 0.0) override;
    /**
     * @brief Render a node given an id and the id of a material at dt time.
     * @param node_id: Node to be rendered.
     * @param material_id: Material id to be used.
     * @param projection: Projection matrix used.
     * @param view: View matrix used.
     * @param dt: Delta time between the beginning of execution and now in seconds.
     */
    void RenderNode(EntityId node_id, EntityId material_id, const glm::mat4& projection,
                    const glm::mat4& view, double dt = 0.0) override;
    /**
     * @brief Render all meshes at a dt time.
     * @param dt: Delta time between the beginning of execution and now in seconds.
     */
    void RenderAllMeshes(const glm::mat4& projection, const glm::mat4& view,
                         double dt = 0.0) override;
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
    LevelInterface& level_;
    EntityId last_program_id_ = NullId;
    Logger& logger_           = Logger::GetInstance();
    // Projection / View / Model matrices.
    glm::mat4 projection_ = glm::mat4(1.0f);
    glm::mat4 view_       = glm::mat4(1.0f);
    glm::mat4 model_      = glm::mat4(1.0f);
    // Viewport top left and bottom right.
    glm::uvec4 viewport_;
    // Frame & Render buffers.
    FrameBuffer frame_buffer_{};
    RenderBuffer render_buffer_{};
    // Display ids.
    EntityId display_program_id_  = 0;
    EntityId display_material_id_ = 0;
    // Texture frame (used in render mesh).
    frame::proto::TextureFrame texture_frame_;
    // The render callback it will be called once per mesh.
    RenderCallback callback_ = [](UniformInterface&, StaticMeshInterface&, MaterialInterface&) {};
};

}  // End namespace frame::opengl.
