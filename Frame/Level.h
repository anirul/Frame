#pragma once

#include <cinttypes>
#include <memory>
#include <utility>

#include "Frame/LevelInterface.h"
#include "Frame/Logger.h"

namespace frame {

/**
 * @class Level
 * @brief This is the level where everything is gathered and organized to be rendered. It is
 * deriving from LevelInterface so you could eventually create a better structure (entity based
 * component).
 */
class Level : public LevelInterface {
   public:
    /**
     * @brief Set default quad static mesh id.
     * @param id: Id of the default quad static mesh.
     */
    void SetDefaultStaticMeshQuadId(EntityId id) final { quad_id_ = id; }
    /**
     * @brief Set default cube static mesh id.
     * @param id: Id of the default cube static mesh.
     */
    void SetDefaultStaticMeshCubeId(EntityId id) final { cube_id_ = id; }
    /**
     * @brief Will get the scene node from an id.
     * @param id: The id to get the scene node from.
     * @return A pointer to the node or null.
     */
    NodeInterface* GetSceneNodeFromId(EntityId id) const override {
        return id_scene_node_map_.at(id).get();
    }
    /**
     * @brief Will get the texture from an id.
     * @param id: The id to get the texture from.
     * @return A pointer to the texture or null.
     */
    TextureInterface* GetTextureFromId(EntityId id) const override {
        return id_texture_map_.at(id).get();
    }
    /**
     * @brief Will get the program from an id.
     * @param id: The id to get the program from.
     * @return A pointer to the program or null.
     */
    ProgramInterface* GetProgramFromId(EntityId id) const override {
        return id_program_map_.at(id).get();
    }
    /**
     * @brief Will get a material from an id.
     * @param id: The id to get the material from.
     * @return A pointer to a material or null.
     */
    MaterialInterface* GetMaterialFromId(EntityId id) const override {
        return id_material_map_.at(id).get();
    }
    /**
     * @brief Will get a buffer from an id.
     * @param id: The id to get the buffer from.
     * @return A pointer to a buffer or null.
     */
    BufferInterface* GetBufferFromId(EntityId id) const override {
        return id_buffer_map_.at(id).get();
    }
    /**
     * @brief Will get a static mesh from an id.
     * @param id: The id to get the static mesh from.
     * @return A pointer to a static mesh or null.
     */
    StaticMeshInterface* GetStaticMeshFromId(EntityId id) const override {
        return id_static_mesh_map_.at(id).get();
    }
    /**
     * @brief Get a vector of static mesh id and corresponding material id.
     * @return Vector of static mesh id and corresponding material id.
     */
    std::vector<std::pair<EntityId, EntityId>> GetStaticMeshMaterialIds() const override {
        return mesh_material_ids_;
    }
    /**
     * @brief Get the default output texture id.
     * @return Id of the default output texture.
     */
    std::optional<EntityId> GetDefaultOutputTextureId() const override {
        return GetIdFromName(default_texture_name_);
    }
    /**
     * @brief Set the default camera name (used during loading as the camera is loaded after).
     * @param name: Name of the camera to be loaded.
     */
    void SetDefaultCameraName(const std::string& name) override { default_camera_name_ = name; }
    /**
     * @brief Get default root scene node id (this is the root of the scene tree).
     * @return An id or an error.
     */
    std::optional<EntityId> GetDefaultRootSceneNodeId() const override {
        return GetIdFromName(default_root_scene_node_name_);
    }
    /**
     * @brief Set the default scene root name (used during loading as the root node is not loaded in
     * a deterministic order).
     * @param name: Name of the scene root.
     */
    void SetDefaultRootSceneNodeName(const std::string& name) override {
        default_root_scene_node_name_ = name;
    }
    /**
     * @brief Get the default camera id, using the name that was stored during loading.
	 * @return An id or an error.
     */
    std::optional<EntityId> GetDefaultCameraId() const override {
        return GetIdFromName(default_camera_name_);
    }
    /**
     * @brief Add a mesh and a material id (used for rendering by mesh later on).
	 * @param node_id: Mesh node id.
	 * @param material_id: Material id.
     */
    void AddMeshMaterialId(EntityId node_id, EntityId material_id) override {
        mesh_material_ids_.emplace_back(node_id, material_id);
    }

   public:
    /**
     * @brief Get the default quad static mesh id.
     * @return The id of the quad static mesh id or error.
     */
    std::optional<EntityId> GetDefaultStaticMeshQuadId() const final;
    /**
     * @brief Get the default cube static mesh id.
     * @return The id of the cube static mesh id or error.
     */
    std::optional<EntityId> GetDefaultStaticMeshCubeId() const final;
    /**
     * @brief Get the id of an element from a name string.
     * @param name: The name string of the element.
     * @return Id of the element or error.
     */
    std::optional<EntityId> GetIdFromName(const std::string& name) const override;
    /**
     * @brief Get the name of an element given an id.
     * @param id: Id of the element to get the name.
     * @return Name of the element.
     */
    std::optional<std::string> GetNameFromId(EntityId id) const override;
    /**
     * @brief Add scene node to the scene tree.
     * @param scene_node: Move a scene node to the scene tree.
     * @return Assigned entity id or error.
     */
    std::optional<EntityId> AddSceneNode(std::unique_ptr<NodeInterface>&& scene_node) override;
    /**
     * @brief Add a texture to the level.
     * @param texture: Move a texture in the level.
     * @return Assigned entity id or error.
     */
    std::optional<EntityId> AddTexture(std::unique_ptr<TextureInterface>&& texture) override;
    /**
     * @brief Add a program to the level.
     * @param program: Move a program in the level.
     * @return Assigned entity id or error.
     */
    std::optional<EntityId> AddProgram(std::unique_ptr<ProgramInterface>&& program) override;
    /**
     * @brief Add a material to the level.
     * @param material: Move a material in the level.
     * @return Assigned entity id or error.
     */
    std::optional<EntityId> AddMaterial(std::unique_ptr<MaterialInterface>&& material) override;
    /**
     * @brief Add a buffer to the level.
     * @param buffer: Move a buffer in the level.
     * @return Assigned entity id or error.
     */
    std::optional<EntityId> AddBuffer(std::unique_ptr<BufferInterface>&& buffer) override;
    /**
     * @brief Add a static mesh to the level.
     * @param static_mesh: Move a buffer in the level.
     * @return Assigned entity id or error.
     */
    std::optional<EntityId> AddStaticMesh(
        std::unique_ptr<StaticMeshInterface>&& static_mesh) override;
    /**
     * @brief Get the list of children from an id in the node list.
     * @param id: The node id you want to get the children.
     * @return The node id children id(s).
     */
    std::optional<std::vector<EntityId>> GetChildList(EntityId id) const override;
    /**
     * @brief Get the parent of a given node id.
     * @param id: The current node we are searching for the parent.
     * @return Parent node id.
     */
    std::optional<EntityId> GetParentId(EntityId id) const override;
    /**
     * @brief Extract a texture (move it) from the level to outside (used in special cases).
	 * @warning This will invalidate this entry!
	 * @param id: The id of the texture to be extracted.
	 * @return A unique pointer to the texture interface.
     */
    std::unique_ptr<TextureInterface> ExtractTexture(EntityId id) override;
    /**
     * @brief Get the default camera from the level.
     * @return A pointer to the default camera.
     */
    Camera* GetDefaultCamera() override;

   protected:
    /**
     * @brief Increase the internal counter and return the value.
	 * @return Current counter + 1.
     */
    EntityId GetTextureNewId() const { return ++next_id_maker_; }
    /**
     * @brief Increase the internal counter and return the value.
     * @return Current counter + 1.
     */
    EntityId GetProgramNewId() const { return ++next_id_maker_; }
    /**
     * @brief Increase the internal counter and return the value.
     * @return Current counter + 1.
     */
    EntityId GetMaterialNewId() const { return ++next_id_maker_; }
    /**
     * @brief Increase the internal counter and return the value.
     * @return Current counter + 1.
     */
    EntityId GetBufferNewId() const { return ++next_id_maker_; }
    /**
     * @brief Increase the internal counter and return the value.
     * @return Current counter + 1.
     */
    EntityId GetStaticMeshNewId() const { return ++next_id_maker_; }
    /**
     * @brief Increase the internal counter and return the value.
     * @return Current counter + 1.
     */
    EntityId GetSceneNodeNewId() const { return ++next_id_maker_; }

   protected:
    Logger& logger_                 = Logger::GetInstance();
    mutable EntityId next_id_maker_ = NullId;
    EntityId quad_id_               = 0;
    EntityId cube_id_               = 0;
    std::string name_;
    std::string default_texture_name_;
    std::string default_root_scene_node_name_;
    std::string default_camera_name_;
    std::set<std::string> string_set_                                            = {};
    std::map<EntityId, std::unique_ptr<NodeInterface>> id_scene_node_map_        = {};
    std::map<EntityId, std::unique_ptr<TextureInterface>> id_texture_map_        = {};
    std::map<EntityId, std::unique_ptr<ProgramInterface>> id_program_map_        = {};
    std::map<EntityId, std::unique_ptr<MaterialInterface>> id_material_map_      = {};
    std::map<EntityId, std::unique_ptr<BufferInterface>> id_buffer_map_          = {};
    std::map<EntityId, std::unique_ptr<StaticMeshInterface>> id_static_mesh_map_ = {};
    std::map<std::string, EntityId> name_id_map_                                 = {};
    std::map<EntityId, std::string> id_name_map_                                 = {};
    std::map<EntityId, EntityTypeEnum> id_enum_map_                              = {};
    std::vector<std::pair<EntityId, EntityId>> mesh_material_ids_                = {};
};

}  // End namespace frame.
