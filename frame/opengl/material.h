#pragma once

#include <iostream>
#include <set>
#include <string>

#include "frame/level_interface.h"
#include "frame/material_interface.h"
#include "frame/opengl/texture.h"

namespace frame::opengl
{

/**
 * @class Material
 * @brief this is the support for material when rendering you need to have a
 * material for each mesh so that you know which program to run on each
 * pixel (fragment shader mostly). For now this is not saving a local copy
 * of the level so you'll have to pass it through.
 */
class Material : public MaterialInterface
{
  public:
    /**
     * @brief This is getting the program id from a level or from the local
     * stored one.
     * @param level: Pointer to the local level.
     * @return Id of the program (can be the linked program).
     */
    EntityId GetProgramId(const LevelInterface* level = nullptr) const override;
    /**
     * @brief This is getting the preprocess program id from a level or from the
     * local stored one.
     * @param level: Pointer to the local level.
     * @return Id of the preprocess program (can be the linked program).
     */
    EntityId GetPreprocessProgramId(
        const LevelInterface* level = nullptr) const override;
    /**
     * @brief Get the inner name that correspond to a texture id.
     * @param id: The id to check for corresponding string.
     * @return The string.
     */
    std::string GetInnerName(EntityId id) const;
    /**
     * @brief Store local program id.
     * @param id: the stored program id.
     */
    void SetProgramId(EntityId id) override;
    /**
     * @brief Store local preprocess program id.
     * @param id: the stored preprocess program id.
     */
    void SetPreprocessProgramId(EntityId id) override;
    /**
     * @brief Store a texture reference associated to a given name.
     * @param id: Texture reference id.
     * @param name: Associated name (shader name).
     */
    bool AddTextureId(EntityId id, const std::string& name) override;
    /**
     * @brief Check if the texture is in the material.
     * @param id: Texture to be checked.
     * @return True if present false otherwise.
     */
    bool HasTextureId(EntityId id) const override;
    /**
     * @brief Remove a texture from the material.
     * @param id: Texture to be removed.
     * @return True if removed false otherwise.
     */
    bool RemoveTextureId(EntityId id) override;
    /**
     * @brief Get texture ids of a material.
     * @return Return the list of texture ids.
     */
    std::vector<EntityId> GetTextureIds() const final;
    /**
     * @brief Enable a texture to be used by the context.
     * @param id: Id of the texture to be enabled.
     * @return Return the name and the binding slot of a texture (to be
     * passed to the program).
     */
    std::pair<std::string, int> EnableTextureId(EntityId id) const override;
    /**
     * @brief Unbind the texture and remove it from the list.
     * @param id: Texture id.
     */
    void DisableTextureId(EntityId id) const override;
    /**
     * @brief Disable all the texture and unbind them.
     */
    void DisableAll() const override;
    // Buffer part.
    std::string GetInnerBufferName(const std::string& name) const override;
    bool AddBufferName(
        const std::string& name, const std::string& inner_name) override;
    std::vector<std::string> GetBufferNames() const override;
    std::string GetInnerNodeName(const std::string& name) const override;
    bool AddNodeName(
        const std::string& name, const std::string& inner_name) override;
    std::vector<std::string> GetNodeNames() const override;

    void SetPreprocessProgramName(const std::string& name)
    {
        preprocess_program_name_ = name;
    }

  private:
    std::map<EntityId, std::string> id_name_map_ = {};
    // Preserve insertion order for buffer bindings to match shader layout
    std::vector<std::pair<std::string, std::string>> buffer_name_vec_ = {};
    std::map<std::string, std::string> name_node_name_map_ = {};
    mutable std::array<EntityId, 32> id_array_ = {};
    mutable EntityId program_id_ = 0;
    mutable EntityId preprocess_program_id_ = 0;
    std::string name_;
    std::string program_name_;
    std::string preprocess_program_name_;
};

} // End namespace frame::opengl.
