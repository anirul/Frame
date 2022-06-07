#pragma once

#include <memory>
#include <optional>
#include <unordered_map>

#include "Frame/BufferInterface.h"
#include "Frame/Camera.h"
#include "Frame/EntityId.h"
#include "Frame/MaterialInterface.h"
#include "Frame/NodeInterface.h"
#include "Frame/ProgramInterface.h"
#include "Frame/StaticMeshInterface.h"
#include "Frame/TextureInterface.h"

namespace frame {

/**
 * @class LevelInterface
 * @brief This is the interface to a level class, a level class is the description of a Scene and
 * all the object you need to render. This include the scene tree the materials the programs and the
 * meshes (for now only static meshes).
 */
class LevelInterface {
   public:
    //! @brief Virtual destructor.
    virtual ~LevelInterface() = default;
    /**
     * @brief Will get the scene node from an id.
     * @param id: The id to get the scene node from.
     * @return A pointer to the node or null.
     */
    virtual NodeInterface* GetSceneNodeFromId(EntityId id) const = 0;
    /**
     * @brief Will get the texture from an id.
     * @param id: The id to get the texture from.
     * @return A pointer to the texture or null.
     */
    virtual TextureInterface* GetTextureFromId(EntityId id) const = 0;
    /**
     * @brief Will get the program from an id.
     * @param id: The id to get the program from.
     * @return A pointer to the program or null.
     */
    virtual ProgramInterface* GetProgramFromId(EntityId id) const = 0;
    /**
     * @brief Will get a material from an id.
     * @param id: The id to get the material from.
     * @return A pointer to a material or null.
     */
    virtual MaterialInterface* GetMaterialFromId(EntityId id) const = 0;
    /**
     * @brief Will get a buffer from an id.
     * @param id: The id to get the buffer from.
     * @return A pointer to a buffer or null.
     */
    virtual BufferInterface* GetBufferFromId(EntityId id) const = 0;
    /**
     * @brief Will get a static mesh from an id.
     * @param id: The id to get the static mesh from.
     * @return A pointer to a static mesh or null.
     */
    virtual StaticMeshInterface* GetStaticMeshFromId(EntityId id) const = 0;
    /**
     * @brief Get a vector of static mesh id and corresponding material id.
     * @return Vector of static mesh id and corresponding material id.
     */
    virtual std::vector<std::pair<EntityId, EntityId>> GetStaticMeshMaterialIds() const = 0;
    /**
     * @brief Get the id of an element from a name string.
     * @param name: The name string of the element.
     * @return Id of the element or error.
     */
    virtual std::optional<EntityId> GetIdFromName(const std::string& name) const = 0;
    /**
     * @brief Get the name of an element given an id.
     * @param id: Id of the element to get the name.
     * @return Name of the element or error.
     */
    virtual std::optional<std::string> GetNameFromId(EntityId id) const = 0;
    /**
     * @brief Get the default output texture id.
     * @return Id of the default output texture.
     */
    virtual std::optional<EntityId> GetDefaultOutputTextureId() const = 0;
    /**
     * @brief Set the default camera name (used during loading as the camera is loaded after).
     * @param name: Name of the camera to be loaded.
     */
    virtual void SetDefaultRootSceneNodeName(const std::string& name) = 0;
    /**
     * @brief Get default root scene node id (this is the root of the scene tree).
     * @return An id or an error.
     */
    virtual std::optional<EntityId> GetDefaultRootSceneNodeId() const = 0;
    /**
     * @brief Set the default scene root name (used during loading as the root node is not loaded in
     * a deterministic order).
     * @param name: Name of the scene root.
     */
    virtual void SetDefaultCameraName(const std::string& name) = 0;
    /**
     * @brief Get the default camera id, using the name that was stored during loading.
     * @return An id or an error.
     */
    virtual std::optional<EntityId> GetDefaultCameraId() const = 0;
    /**
     * @brief Get the list of children from an id in the node list.
     * @param id: The node id you want to get the children.
     * @return The node id children id(s).
     */
    virtual std::optional<std::vector<EntityId>> GetChildList(const EntityId id) const = 0;
    /**
     * @brief Get the parent of a given node id.
     * @param id: The current node we are searching for the parent.
     * @return Parent node id.
     */
    virtual std::optional<EntityId> GetParentId(EntityId id) const = 0;
    /**
     * @brief Get the default quad static mesh id.
     * @return The id of the quad static mesh id or error.
     */
    virtual std::optional<EntityId> GetDefaultStaticMeshQuadId() const = 0;
    /**
     * @brief Set default quad static mesh id.
     * @param id: Id of the default quad static mesh.
     */
    virtual void SetDefaultStaticMeshQuadId(EntityId id) = 0;
    /**
     * @brief Get the default cube static mesh id.
     * @return The id of the cube static mesh id or error.
     */
    virtual std::optional<EntityId> GetDefaultStaticMeshCubeId() const = 0;
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
    virtual std::optional<EntityId> AddSceneNode(std::unique_ptr<NodeInterface>&& scene_node) = 0;
    /**
     * @brief Add a texture to the level.
     * @param texture: Move a texture in the level.
     * @return Assigned entity id or error.
     */
    virtual std::optional<EntityId> AddTexture(std::unique_ptr<TextureInterface>&& texture) = 0;
    /**
     * @brief Add a program to the level.
     * @param program: Move a program in the level.
     * @return Assigned entity id or error.
     */
    virtual std::optional<EntityId> AddProgram(std::unique_ptr<ProgramInterface>&& program) = 0;
    /**
     * @brief Add a material to the level.
     * @param material: Move a material in the level.
     * @return Assigned entity id or error.
     */
    virtual std::optional<EntityId> AddMaterial(std::unique_ptr<MaterialInterface>&& material) = 0;
    /**
     * @brief Add a buffer to the level.
     * @param buffer: Move a buffer in the level.
     * @return Assigned entity id or error.
     */
    virtual std::optional<EntityId> AddBuffer(std::unique_ptr<BufferInterface>&& buffer) = 0;
    /**
     * @brief Add a static mesh to the level.
     * @param static_mesh: Move a buffer in the level.
     * @return Assigned entity id or error.
     */
    virtual std::optional<EntityId> AddStaticMesh(
        std::unique_ptr<StaticMeshInterface>&& static_mesh) = 0;
    /**
     * @brief Add a mesh and a material id (used for rendering by mesh later on).
     * @param node_id: Mesh node id.
     * @param material_id: Material id.
     */
    virtual void AddMeshMaterialId(EntityId node_id, EntityId material_id) = 0;
    /**
     * @brief Extract a texture (move it) from the level to outside (used in special cases).
     * @warning This will invalidate this entry!
     * @param id: The id of the texture to be extracted.
     * @return A unique pointer to the texture interface.
     */
    virtual std::unique_ptr<TextureInterface> ExtractTexture(EntityId id) = 0;
    /**
     * @brief Get the default camera from the level.
     * @return A pointer to the default camera.
     */
    virtual Camera* GetDefaultCamera() = 0;
};

}  // End namespace frame.