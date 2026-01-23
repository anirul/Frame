#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <optional>

#include "frame/camera_interface.h"
#include "frame/material_interface.h"
#include "frame/static_mesh_interface.h"

namespace frame
{

/**
 * @class RendererInterface
 * @brief This is the renderer class this is the class that is doing the
 * rendering part.
 */
struct RendererInterface
{
    //! Virtual destructor.
    virtual ~RendererInterface() = default;
    /**
     * @brief Set the delta time for renderer.
     * @param dt: Delta time between the beginning of execution and now in
     * seconds.
     */
    virtual void SetDeltaTime(double dt) = 0;
    /**
     * @brief Set the cubemap target(used in the render mesh method).
     * @param texture_frame: The proto describing the cube map.
     */
    virtual void SetCubeMapTarget(frame::proto::TextureFrame texture_frame) = 0;
    /**
     * @brief Set the current viewport.
     * @param viewport: New viewport.
     */
    virtual void SetViewport(glm::uvec4 viewport) = 0;
    /**
	 * @brief Pre render path this is supposed to be called once.
	 */
    virtual void PreRender() = 0;
    /**
     * @brief Render the skybox.
     * @param camera: The camera to be used for rendering.
         */
    virtual void RenderSkybox(const CameraInterface& camera) = 0;
    /**
     * @brief Render all meshes at a dt time.
     * @param camera: The camera to be used for rendering.
     */
    virtual void RenderScene(const CameraInterface& camera) = 0;
    /**
     * @brief Post process the scene at dt time.
	 */
	virtual void PostProcess() = 0;
    /**
     * @brief Display to the screen at dt time.
     */
    virtual void PresentFinal() = 0;
    /**
     * @brief Set depth test.
     * @param enable: Enable or disable depth test.
     */
    virtual void SetDepthTest(bool enable) = 0;
	/**
     * @brief Render to a mesh at a dt time.
     * @param static_mesh: Static mesh to render.
     * @param material: Material to be used (or null).
     * @param projection: Projection matrix used.
     * @param view: View matrix used.
     * @param model: Model matrix to be used.
     */
    virtual void RenderMesh(
        StaticMeshInterface& static_mesh,
        MaterialInterface& material,
        const glm::mat4& projection,
        const glm::mat4& view = glm::mat4(1.0f),
        const glm::mat4& model_mat = glm::mat4(1.0f)) = 0;
    /**
     * @brief Render a node given an id and the id of a material at dt time.
     * @param node_id: Node to be rendered.
     * @param material_id: Material id to be used.
     * @param projection: Projection matrix used.
     * @param view: View matrix used.
     * @return the computed model matrix.
     */
    virtual std::optional<glm::mat4> RenderNode(
        EntityId node_id,
        EntityId material_id,
        const glm::mat4& projection,
        const glm::mat4& view) = 0;
    /**
     * @brief Render callback is a callback that is called once per mesh.
     * @param uniform: A list of uniform for the mesh.
     * @param device: The device for the mesh rendering.
     * @param mesh: The render mesh as a ref.
     * @param material: The render material ref.
     */
    using RenderCallback =
		std::function<void(
			UniformCollectionInterface&,
			StaticMeshInterface&,
			MaterialInterface&)>;
    /**
     * @brief Set the mesh render callback.
     * @param callback: The callback to be added to the render.
     */
    virtual void SetMeshRenderCallback(RenderCallback callback) = 0;
};

} // End namespace frame.
