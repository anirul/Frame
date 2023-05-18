#pragma once

#include <glm/glm.hpp>

#include "frame/material_interface.h"
#include "frame/static_mesh_interface.h"

namespace frame {

/**
 * @class RendererInterface
 * @brief This is the renderer class this is the class that is doing the
 * rendering part.
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
   * @brief Set the default model matrix, can be changed by render mesh (move
   * elision)!
   * @param projection: The model matrix.
   */
  virtual void SetModel(glm::mat4 model) = 0;
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
   * @brief Render to a mesh at a dt time.
   * @param static_mesh: Static mesh to render.
   * @param material: Material to be used (or null).
   * @param projection: Projection matrix used.
   * @param view: View matrix used.
   * @param model: Model matrix to be used.
   * @param dt: Delta time between the beginning of execution and now in
   * seconds.
   */
  virtual void RenderMesh(StaticMeshInterface& static_mesh,
                          MaterialInterface& material,
                          const glm::mat4& projection,
                          const glm::mat4& view = glm::mat4(1.0f),
                          const glm::mat4& model_mat = glm::mat4(1.0f),
                          double dt = 0.0) = 0;
  /**
   * @brief Render a node given an id and the id of a material at dt time.
   * @param node_id: Node to be rendered.
   * @param material_id: Material id to be used.
   * @param projection: Projection matrix used.
   * @param view: View matrix used.
   * @param dt: Delta time between the beginning of execution and now in
   * seconds.
   */
  virtual void RenderNode(EntityId node_id, EntityId material_id,
                          const glm::mat4& projection, const glm::mat4& view,
                          double dt = 0.0) = 0;
  /**
   * @brief Render all meshes at a dt time.
   * @param dt: Delta time between the beginning of execution and now in
   * seconds.
   */
  virtual void RenderAllMeshes(const glm::mat4& projection,
                               const glm::mat4& view, double dt = 0.0) = 0;
  /**
   * @brief Display to the screen at dt time.
   * @param dt: Delta time between the beginning of execution and now in
   * seconds.
   */
  virtual void Display(double dt = 0.0) = 0;
  /**
   * @brief Set depth test.
   * @param enable: Enable or disable depth test.
   */
  virtual void SetDepthTest(bool enable) = 0;
  /**
   * @brief Render callback is a callback that is called once per mesh.
   * @param uniform: A list of uniform for the mesh.
   * @param device: The device for the mesh rendering.
   * @param mesh: The render mesh as a ref.
   * @param material: The render material ref.
   */
  using RenderCallback = std::function<void(
      UniformInterface&, StaticMeshInterface&, MaterialInterface&)>;
  /**
   * @brief Set the mesh render callback.
   * @param callback: The callback to be added to the render.
   */
  virtual void SetMeshRenderCallback(RenderCallback callback) = 0;
};

}  // End namespace frame.
