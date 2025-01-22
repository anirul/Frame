#pragma once

#include <memory>

#include "frame/opengl/frame_buffer.h"
#include "frame/opengl/render_buffer.h"
#include "frame/program_interface.h"
#include "frame/renderer_interface.h"
#include "frame/static_mesh_interface.h"
#include "frame/uniform_interface.h"
#include "frame/window_interface.h"

namespace frame::opengl
{

/**
 * @class Renderer
 * @brief This is the renderer class this is the class that is doing the
 * rendering part.
 *
 * Thinking about removing the level dependency and passing it as a parameter?
 */
class Renderer : public RendererInterface
{
  public:
    /**
     * @brief Constructor
     * @param level: The level to render.
     * @param viewport: The viewport.
     */
    Renderer(LevelInterface& level, glm::uvec4 viewport);

  public:
    /**
     * @brief Set the delta time for renderer.
     * @param dt: Delta time between the beginning of execution and now in
     * seconds.
     */
    void SetDeltaTime(double dt) override
    {
        delta_time_ = dt;
    }
    /**
     * @brief Set the cubemap target(used in the render mesh method).
     * @param texture_frame: The proto describing the cube map.
     */
    void SetCubeMapTarget(frame::proto::TextureFrame texture_frame) override
    {
        texture_frame_ = texture_frame;
    }
    /**
     * @brief Set the current viewport.
     * @param viewport: New viewport.
     */
    void SetViewport(glm::uvec4 viewport) override
    {
        viewport_ = viewport;
    }
    /**
     * @brief Add a mesh render callback.
     * @param callback: The callback to be added to the render.
     */
    void SetMeshRenderCallback(RenderCallback callback) override
    {
        callback_ = callback;
    }

  public:
    /**
     * @brief Set depth test.
     * @param enable: Enable or disable depth test.
     */
    void SetDepthTest(bool enable) override;
    /**
     * @brief Render the shadows.
     * @param camera: The camera to be used for rendering.
     */
    void PreRender() override;
    /**
     * @brief Render the shadows.
     * @param camera: The camera to be used for rendering.
     */
    void RenderShadows(const CameraInterface& camera) override;
    /**
     * @brief Render the skybox.
     * @param camera: The camera to be used for rendering.
     */
    void RenderSkybox(const CameraInterface& camera) override;
    /**
     * @brief Render all meshes at a dt time.
     * @param camera: The camera to be used for rendering.
     */
    void RenderScene(const CameraInterface& camera) override;
    /**
     * @brief Post process the scene at dt time.
     */
    void PostProcess() override;
    /**
     * @brief Display to the screen at dt time.
     */
    void PresentFinal() override;

  public:
    /**
     * @brief Render to a mesh at a dt time.
     * @param static_mesh: Static mesh to render.
     * @param material: Material to be used (or null).
     * @param model_mat: Model matrix to be used.
     */
    void RenderMesh(
        StaticMeshInterface& static_mesh,
        MaterialInterface& material,
        const glm::mat4& projection,
        const glm::mat4& view = glm::mat4(1.0f),
        const glm::mat4& model = glm::mat4(1.0f)) override;
    /**
     * @brief Render a node given an id and the id of a material at dt time.
     * @param node_id: Node to be rendered.
     * @param material_id: Material id to be used.
     * @param projection: Projection matrix used.
     * @param view: View matrix used.
	 * @return the computed model matrix.
     */
    std::optional<glm::mat4> RenderNode(
        EntityId node_id,
        EntityId material_id,
        const glm::mat4& projection,
        const glm::mat4& view) override;

  private:
    LevelInterface& level_;
    double delta_time_ = 0.0;
    proto::SceneStaticMesh::RenderTimeEnum render_time_ =
        proto::SceneStaticMesh::SCENE_RENDER_TIME;
    Logger& logger_ = Logger::GetInstance();
    glm::mat4 env_map_model_ = glm::mat4(1.0f);
    // Viewport top left and bottom right.
    glm::uvec4 viewport_;
    // Frame & Render buffers.
    std::unique_ptr<FrameBuffer> frame_buffer_{nullptr};
    std::unique_ptr<RenderBuffer> render_buffer_{nullptr};
    // Display ids.
    EntityId display_program_id_ = 0;
    EntityId display_material_id_ = 0;
    // Texture frame (used in render mesh).
    frame::proto::TextureFrame texture_frame_;
    bool first_render_ = true;
    // The render callback it will be called once per mesh.
    RenderCallback callback_ =
        [](UniformInterface&, StaticMeshInterface&, MaterialInterface&) {};
};

} // End namespace frame::opengl.
