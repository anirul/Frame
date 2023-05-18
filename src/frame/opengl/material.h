#pragma once

#include <iostream>
#include <set>
#include <string>

#include "frame/level_interface.h"
#include "frame/material_interface.h"
#include "frame/opengl/texture.h"

namespace frame::opengl {

/**
 * @class Material
 * @brief this is the support for material when rendering you need to have a
 * material for each mesh so that you know which program to run on each pixel
 * (fragment shader mostly). For now this is not saving a local copy of the
 * level so you'll have to pass it through.
 */
class Material : public MaterialInterface {
 public:
  /**
   * @brief This is getting the program id from a level or from the local stored
   * one.
   * @param level: Pointer to the local level.
   * @return Id of the program (can be the linked program).
   */
  EntityId GetProgramId(const LevelInterface* level = nullptr) const override;
  /**
   * @brief Store local program id.
   * @param id: the stored program id.
   */
  void SetProgramId(EntityId id) override;
  /**
   * @brief Store the program name.
   * @param name: Program name.
   */
  void SetProgramName(const std::string& name) override;
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
   * @brief Get ids of a material.
   * @return Return the list of texture ids.
   */
  const std::vector<EntityId> GetIds() const final;
  /**
   * @brief Enable a texture to be used by the context.
   * @param id: Id of the texture to be enabled.
   * @return Return the name and the binding slot of a texture (to be passed to
   * the program).
   */
  const std::pair<std::string, int> EnableTextureId(EntityId id) const override;
  /**
   * @brief Unbind the texture and remove it from the list.
   * @param id: Texture id.
   */
  void DisableTextureId(EntityId id) const override;
  /**
   * @brief Disable all the texture and unbind them.
   */
  void DisableAll() const override;
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

 private:
  std::map<EntityId, std::string> id_name_map_ = {};
  mutable std::array<EntityId, 32> id_array_ = {};
  mutable EntityId program_id_ = 0;
  std::string name_;
  std::string program_name_;
};

}  // End namespace frame::opengl.
