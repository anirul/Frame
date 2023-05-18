#pragma once

#include <array>
#include <initializer_list>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "frame/level_interface.h"
#include "frame/opengl/bind_interface.h"
#include "frame/opengl/buffer.h"
#include "frame/opengl/material.h"
#include "frame/opengl/program.h"
#include "frame/opengl/texture.h"
#include "frame/static_mesh_interface.h"

namespace frame::opengl {

/**
 * @class StaticMesh
 * @brief A static mesh is a mesh that cannot change over time (no skeleton).
 */
class StaticMesh : public BindInterface, public StaticMeshInterface {
 public:
  /**
   * @brief Create a mesh from a static mesh config struct.
   * @param level: The level into witch the class will be generated.
   * @param config: The static mesh config structure.
   */
  StaticMesh(LevelInterface& level, const StaticMeshParameter& parameters);
  //! @brief Virtual destructor.
  virtual ~StaticMesh();

 public:
  /**
   * @brief Get point buffer id.
   * @return Current point buffer id.
   */
  EntityId GetPointBufferId() const override { return point_buffer_id_; }
  /**
   * @brief Get color buffer id.
   * @return Current color buffer id.
   */
  EntityId GetColorBufferId() const override { return color_buffer_id_; }
  /**
   * @brief Get normal buffer id.
   * @return Current normal buffer id.
   */
  EntityId GetNormalBufferId() const override { return normal_buffer_id_; }
  /**
   * @brief Get texture buffer id.
   * @return Current texture buffer id.
   */
  EntityId GetTextureBufferId() const override { return texture_buffer_id_; }
  /**
   * @brief Get index buffer id (triangle based).
   * @return Current index buffer id.
   */
  EntityId GetIndexBufferId() const override { return index_buffer_id_; }
  /**
   * @brief This is the size in bytes! so if you need the element size just
   * divide this number by the sizeof(std::int32_t).
   * @return Size of the index buffer in bytes!
   */
  std::size_t GetIndexSize() const override { return index_size_; }
  /**
   * @brief Update the internals to the stream values.
   * @param level: A pointer to the current level.
   */
  void SetIndexSize(std::size_t index_size) override {
    index_size_ = index_size;
  }
  /**
   * @brief Set the way a mesh is rendered (point/line/triangle) triangle is the
   * default.
   * @param render_enum: The basic shape of the renderer that should be used on
   * this mesh.
   */
  void SetRenderPrimitive(proto::SceneStaticMesh::RenderPrimitiveEnum
                              render_primitive_enum) override {
    render_primitive_enum_ = render_primitive_enum;
  }
  /**
   * @brief Get the static mesh render primitive.
   * @return Get the render primitive.
   */
  proto::SceneStaticMesh::RenderPrimitiveEnum GetRenderPrimitive()
      const override {
    return render_primitive_enum_;
  }
  //! @brief Lock the bind for RAII interface to the bind interface.
  void LockedBind() const override { locked_bind_ = true; }
  //! @brief Unlock the bind for RAII interface to the bind interface.
  void UnlockedBind() const override { locked_bind_ = false; }
  /**
   * @brief From bind interface this return the id of the internal OpenGL
   * object.
   * @return Id of the OpenGL object.
   */
  unsigned int GetId() const override { return vertex_array_object_; }
  /**
   * @brief Check if depth buffer is cleared or not.
   * @return Is depth buffer cleared?
   */
  bool IsClearBuffer() const override { return clear_depth_buffer_; }
  /**
   * @brief Get name from the name interface.
   * @return The name of the object.
   */
  std::string GetName() const override { return name_; }
  /**
   * @brief Set name from the name interface.
   * @param name: New name to be set.
   */
  void SetName(const std::string& name) override { name_ = name; }

 public:
  /**
   * @brief From the bind interface this will bind the mesh to the current
   * context the slot parameter can be forgotten as it has no sens with frame
   * buffers.
   * @param slot: Should be ignored.
   */
  void Bind(const unsigned int slot = 0) const override;
  //! @brief From the bind interface this will unbind the current frame buffer
  //! from the context.
  void UnBind() const override;

 protected:
  LevelInterface& level_;
  bool clear_depth_buffer_ = true;
  mutable bool locked_bind_ = false;
  EntityId point_buffer_id_ = NullId;
  std::uint32_t point_buffer_size_ = 3;
  EntityId color_buffer_id_ = NullId;
  std::uint32_t color_buffer_size_ = 3;
  EntityId normal_buffer_id_ = NullId;
  std::uint32_t normal_buffer_size_ = 3;
  EntityId texture_buffer_id_ = NullId;
  std::uint32_t texture_buffer_size_ = 2;
  EntityId index_buffer_id_ = NullId;
  std::size_t index_size_ = 0;
  unsigned int vertex_array_object_ = 0;
  proto::SceneStaticMesh::RenderPrimitiveEnum render_primitive_enum_ = {};
  float point_size_ = 1.0f;
  std::string name_;
};

/**
 * @brief Create a quad static mesh (this will be use for texture effects).
 * @param level: The quad static mesh will be added to this level.
 * @return Will return an entity id if successful.
 */
EntityId CreateQuadStaticMesh(LevelInterface& level);
/**
 * @brief Create a cube static mesh center around the origin of space (this will
 * be use to map a cubemap).
 * @param level: The cube static mesh will be added to this level.
 * @return Will return an entity id if successful.
 */
EntityId CreateCubeStaticMesh(LevelInterface& level);

}  // End namespace frame::opengl.
