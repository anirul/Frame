#pragma once

#include <memory>
#include <optional>
#include <unordered_map>

#include "frame/buffer_interface.h"
#include "frame/camera_interface.h"
#include "frame/entity_id.h"
#include "frame/light_interface.h"
#include "frame/material_interface.h"
#include "frame/node_interface.h"
#include "frame/program_interface.h"
#include "frame/static_mesh_interface.h"
#include "frame/texture_interface.h"

namespace frame
{

/**
 * @class LevelInterface
 * @brief This is the interface to a level class, a level class is the
 *        description of a Scene and all the object you need to render.
 *
 * This include the scene tree the materials the programs and the meshes
 * (for now only static meshes).
 */
class LevelInterface : public NameInterface
{
  public:
    //! @brief Virtual destructor.
    virtual ~LevelInterface() = default;
    /**
     * @brief Will get the scene node from an id.
     * @param id: The id to get the scene node from.
     * @return A pointer to the node or null.
     */
    virtual NodeInterface& GetSceneNodeFromId(EntityId id) const = 0;
    /**
     * @brief Will get the texture from an id.
     * @param id: The id to get the texture from.
     * @return A pointer to the texture or null.
     */
    virtual TextureInterface& GetTextureFromId(EntityId id) const = 0;
    /**
     * @brief Will get the program from an id.
     * @param id: The id to get the program from.
     * @return A pointer to the program or null.
     */
    virtual ProgramInterface& GetProgramFromId(EntityId id) const = 0;
    /**
     * @brief Will get a material from an id.
     * @param id: The id to get the material from.
     * @return A pointer to a material or null.
     */
    virtual MaterialInterface& GetMaterialFromId(EntityId id) const = 0;
    /**
     * @brief Will get a buffer from an id.
     * @param id: The id to get the buffer from.
     * @return A pointer to a buffer or null.
     */
    virtual BufferInterface& GetBufferFromId(EntityId id) const = 0;
    /**
     * @brief Will get a static mesh from an id.
     * @param id: The id to get the static mesh from.
     * @return A pointer to a static mesh or null.
     */
    virtual StaticMeshInterface& GetStaticMeshFromId(EntityId id) const = 0;
    /**
     * @brief Get all light from the level.
     * @param id: The id to get the light from.
     * @return A reference to a light.
     */
    virtual LightInterface& GetLightFromId(EntityId id) const = 0;
    /**
     * @brief Get a vector of static mesh id and corresponding material id.
     * @return Vector of static mesh id and corresponding material id.
     */
    virtual std::vector<std::pair<EntityId, EntityId>> GetStaticMeshMaterialIds(
        proto::SceneStaticMesh::RenderTimeEnum render_time_enum =
            proto::SceneStaticMesh::SCENE_RENDER_TIME) const = 0;
    /**
     * @brief Get the id of an element from a name string.
     * @param name: The name string of the element.
     * @return Id of the element or error.
     */
    virtual EntityId GetIdFromName(const std::string& name) const = 0;
    /**
     * @brief Get the name of an element given an id.
     * @param id: Id of the element to get the name.
     * @return Name of the element or error.
     */
    virtual std::string GetNameFromId(EntityId id) const = 0;
    /**
     * @brief Get the default output texture id.
     * @return Id of the default output texture.
     */
    virtual EntityId GetDefaultOutputTextureId() const = 0;
    /**
     * @brief Set the default camera name (used during loading as the camera
     *        is loaded after).
     * @param name: Name of the camera to be loaded.
     */
    virtual void SetDefaultRootSceneNodeName(const std::string& name) = 0;
    /**
     * @brief Get default root scene node id (this is the root of the scene
     *        tree).
     * @return An id or an error.
     */
    virtual EntityId GetDefaultRootSceneNodeId() const = 0;
    /**
     * @brief Set the default scene root name (used during loading as the
     *        root node is not loaded in a deterministic order).
     * @param name: Name of the scene root.
     */
    virtual void SetDefaultCameraName(const std::string& name) = 0;
    /**
     * @brief Set the default texture name.
     * @param name: Name of the scene root.
     */
    virtual void SetDefaultTextureName(const std::string& name) = 0;
    /**
     * @brief Get the default camera id, using the name that was stored
     *        during loading.
     * @return An id or an error.
     */
    virtual EntityId GetDefaultCameraId() const = 0;
    /**
     * @brief Get the default material id of the shadow material.
     * @return An id or an error.
     */
    virtual EntityId GetDefaultShadowMaterialId() const = 0;
    /**
     * @brief Set the default material id of the shadow material.
     * @param id: Id of the shadow material.
     */
    virtual void SetDefaultShadowMaterialId(EntityId id) = 0;
    /**
     * @brief Get the list of children from an id in the node list.
     * @param id: The node id you want to get the children.
     * @return The node id children id(s).
     */
    virtual std::vector<EntityId> GetChildList(const EntityId id) const = 0;
    /**
     * @brief Get the parent of a given node id.
     * @param id: The current node we are searching for the parent.
     * @return Parent node id.
     */
    virtual EntityId GetParentId(EntityId id) const = 0;
    /**
     * @brief Get the default quad static mesh id.
     * @return The id of the quad static mesh id or error.
     */
    virtual EntityId GetDefaultStaticMeshQuadId() const = 0;
    /**
     * @brief Set default quad static mesh id.
     * @param id: Id of the default quad static mesh.
     */
    virtual void SetDefaultStaticMeshQuadId(EntityId id) = 0;
    /**
     * @brief Get the default cube static mesh id.
     * @return The id of the cube static mesh id or error.
     */
    virtual EntityId GetDefaultStaticMeshCubeId() const = 0;
    /**
     * @brief Set default cube static mesh id.
     * @param id: Id of the default cube static mesh.
     */
    virtual void SetDefaultStaticMeshCubeId(EntityId id) = 0;
    /**
     * @brief Add scene node to the scene tree.
     * @param scene_node: Move a scene node to the scene tree.
     * @return Assigned entity id or error.
     */
    virtual EntityId AddSceneNode(
        std::unique_ptr<NodeInterface>&& scene_node) = 0;
    /**
     * @brief Add a texture to the level.
     * @param texture: Move a texture in the level.
     * @return Assigned entity id or error.
     */
    virtual EntityId AddTexture(
        std::unique_ptr<TextureInterface>&& texture) = 0;
    /**
     * @brief Add a program to the level.
     * @param program: Move a program in the level.
     * @return Assigned entity id or error.
     */
    virtual EntityId AddProgram(
        std::unique_ptr<ProgramInterface>&& program) = 0;
    /**
     * @brief Add a material to the level.
     * @param material: Move a material in the level.
     * @return Assigned entity id or error.
     */
    virtual EntityId AddMaterial(
        std::unique_ptr<MaterialInterface>&& material) = 0;
    /**
     * @brief Add a buffer to the level.
     * @param buffer: Move a buffer in the level.
     * @return Assigned entity id or error.
     */
    virtual EntityId AddBuffer(std::unique_ptr<BufferInterface>&& buffer) = 0;
    /**
     * @brief Remove a buffer from the level.
     * @param buffer: The buffer id to be removed.
     */
    virtual void RemoveBuffer(EntityId buffer) = 0;
    /**
     * @brief Add a static mesh to the level.
     * @param static_mesh: Move a buffer in the level.
     * @return Assigned entity id or error.
     */
    virtual EntityId AddStaticMesh(
        std::unique_ptr<StaticMeshInterface>&& static_mesh) = 0;
    /**
     * @brief Add a light to the level.
     * @param light: Move a light in the level.
     * @return Assigned entity id or error.
     */
    virtual EntityId AddLight(std::unique_ptr<LightInterface>&& light) = 0;
    /**
     * @brief Add a mesh and a material id (used for rendering by mesh later
     *        on).
     * @param node_id: Mesh node id.
     * @param material_id: Material id.
     */
    virtual void AddMeshMaterialId(
        EntityId node_id,
        EntityId material_id,
        proto::SceneStaticMesh::RenderTimeEnum render_time_enum =
            proto::SceneStaticMesh::SCENE_RENDER_TIME) = 0;
    /**
     * @brief Get all texture from the level.
     * @return A vector of texture ids.
     */
    virtual std::vector<EntityId> GetTextures() const = 0;
    /**
     * @brief Get all light from the level.
     * @return A vector of light ids.
     */
    virtual std::vector<EntityId> GetLights() const = 0;
    /**
     * @brief Get all the program from the level.
     * @return A vector of program ids.
     */
    virtual std::vector<EntityId> GetPrograms() const = 0;
    /**
     * @brief Get all the material from the level.
     * @return A vector of material ids.
     */
    virtual std::vector<EntityId> GetMaterials() const = 0;
    /**
     * @brief Extract a texture (move it) from the level to outside (used in
     * special cases).
     * @warning This will invalidate this entry!
     * @param id: The id of the texture to be extracted.
     * @return A unique pointer to the texture interface.
     */
    virtual std::unique_ptr<TextureInterface> ExtractTexture(EntityId id) = 0;
    /**
     * @brief Get the default camera from the level.
     * @return A pointer to the default camera.
     */
    virtual CameraInterface& GetDefaultCamera() = 0;
    /**
     * @brief Get enum type from Id.
     * @param id: Id to be returned.
     * @return An enum type.
     */
    virtual EntityTypeEnum GetEnumTypeFromId(EntityId id) const = 0;
    /**
     * @brief Replace a texture to a new texture given as a vector.
     * @param vector: The new vector containing the new texture.
     * @param size: Size of the new texture.
     * @param bytes_per_pixels: Byte per pixel in the new texture.
     * @param id: The id of the texture to be replaced.
     */
    virtual void ReplaceTexture(
        std::vector<std::uint8_t>&& vector,
        glm::uvec2 size,
        std::uint8_t bytes_per_pixel,
        EntityId id) = 0;
    /**
     * @brief Replace a mesh in the mesh vector in the entity management
     *        element.
     * @param mesh: A unique pointer to the mesh.
     * @param id: The id to replace the mesh.
     */
    virtual void ReplaceMesh(
        std::unique_ptr<StaticMeshInterface>&& mesh, EntityId id) = 0;
};

} // End namespace frame.
