#pragma once

#include <array>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Frame/LevelInterface.h"
#include "Frame/OpenGL/BindInterface.h"
#include "Frame/OpenGL/Buffer.h"
#include "Frame/OpenGL/Material.h"
#include "Frame/OpenGL/Program.h"
#include "Frame/OpenGL/Texture.h"
#include "Frame/StaticMeshInterface.h"

namespace frame::opengl {

/**
 * @class StaticMesh
 * @brief A static mesh is a mesh that cannot change over time (no skeleton).
 */
class StaticMesh : public BindInterface, public StaticMeshInterface {
   public:
    /**
     * @brief Create a mesh from a set of vectors.
     * @param level: The level into witch the mesh will be stored.
     * @param point_buffer_id: A buffer that contain the point to the mesh.
     * @param normal_buffer_id: A buffer that contain the normal to the mesh.
     * @param texture_buffer_id: A buffer that contain the u,v information to the mesh.
     * @param index_buffer_id: A buffer that contain the index of the triangles from the mesh.
     * @param material_id: An id to the material used to render the mesh.
     */
    StaticMesh(LevelInterface* level, EntityId point_buffer_id, EntityId normal_buffer_id,
               EntityId texture_buffer_id, EntityId index_buffer_id, EntityId material_id = 0);
    //! @brief Virtual destructor.
    virtual ~StaticMesh();

   public:
    /**
     * @brief Set the material id.
     * @param id: the material id to be set.
     */
    void SetMaterialId(EntityId id) override { material_id_ = id; }
    /**
     * @brief Get material id.
     * @return Current material id.
     */
    EntityId GetMaterialId() const override { return material_id_; }
    /**
     * @brief Get point buffer id.
     * @return Current point buffer id.
     */
    EntityId GetPointBufferId() const override { return point_buffer_id_; }
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
     * @brief This is the size in bytes! so if you need the element size just divide this number by
     * the sizeof(std::int32_t).
     * @return Size of the index buffer in bytes!
     */
    std::size_t GetIndexSize() const override { return index_size_; }
    //! @brief Lock the bind for RAII interface to the bind interface.
    void LockedBind() const override { locked_bind_ = true; }
    //! @brief Unlock the bind for RAII interface to the bind interface.
    void UnlockedBind() const override { locked_bind_ = false; }
    /**
     * @brief From bind interface this return the id of the internal OpenGL object.
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
     * @brief From the bind interface this will bind the mesh to the current context the slot
     * parameter can be forgotten as it has no sens with frame buffers.
     * @param slot: Should be ignored.
     */
    void Bind(const unsigned int slot = 0) const override;
    //! @brief From the bind interface this will unbind the current frame buffer from the context.
    void UnBind() const override;

   protected:
    bool clear_depth_buffer_          = true;
    mutable bool locked_bind_         = false;
    EntityId material_id_             = 0;
    EntityId point_buffer_id_         = 0;
    EntityId normal_buffer_id_        = 0;
    EntityId texture_buffer_id_       = 0;
    EntityId index_buffer_id_         = 0;
    std::size_t index_size_           = 0;
    unsigned int vertex_array_object_ = 0;
    std::string name_;
};

/**
 * @brief Create a quad static mesh (this will be use for texture effects).
 * @param level: The quad static mesh will be added to this level.
 * @return Will return an entity id if successful.
 */
std::optional<EntityId> CreateQuadStaticMesh(LevelInterface* level);
/**
 * @brief Create a cube static mesh center around the origin of space (this will be use to map a
 * cubemap).
 * @param level: The cube static mesh will be added to this level.
 * @return Will return an entity id if successful.
 */
std::optional<EntityId> CreateCubeStaticMesh(LevelInterface* level);

}  // End namespace frame::opengl.
