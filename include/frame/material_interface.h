#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "frame/entity_id.h"
#include "frame/serialize.h"

namespace frame
{

class LevelInterface;

/**
 * @class MaterialInterface
 * @brief this is the support for material when rendering you need to have a
 * material for each mesh so that you know which program to run on each pixel
 * (fragment shader mostly).
 *
 * For now this is not saving a local copy of the level so you'll have to
 * pass it through.
 */
class MaterialInterface : public Serialize<proto::Material>
{
  public:
    //! @brief Virtual destructor.
    virtual ~MaterialInterface() = default;
    /**
     * @brief This is getting the program id from a level or from the local
     * stored one.
     * @param level: Pointer to the local level.
     * @return Id of the program (can be the linked program).
     */
    virtual EntityId GetProgramId(
        const LevelInterface* level = nullptr) const = 0;
    /**
     * @brief Get the inner name that correspond to a texture id.
     * @param id: The id to check for corresponding string.
     * @return The string.
     */
    virtual std::string GetInnerName(EntityId id) const = 0;
    /**
     * @brief Store local program id.
     * @param id: the stored program id.
     */
    virtual void SetProgramId(EntityId id) = 0;
    /**
     * @brief Store a texture reference associated to a given name.
     * @param id: Texture reference id.
     * @param name: Associated name (shader name).
     */
    virtual bool AddTextureId(EntityId id, const std::string& name) = 0;
    /**
     * @brief Check if the texture is in the material.
     * @param id: Texture to be checked.
     * @return True if present false otherwise.
     */
    virtual bool HasTextureId(EntityId id) const = 0;
    /**
     * @brief Remove a texture from the material.
     * @param id: Texture to be removed.
     * @return True if removed false otherwise.
     */
    virtual bool RemoveTextureId(EntityId id) = 0;
    /**
     * @brief Get texture ids of a material.
     * @return Return the list of texture ids.
     */
    virtual std::vector<EntityId> GetTextureIds() const = 0;
    /**
     * @brief Get the inner name that correspond to a buffer id.
     * @param id: The id to check for corresponding string.
     * @return The string.
     */
    virtual std::string GetInnerBufferName(EntityId id) const = 0;
    /**
     * @brief Store a buffer reference associated to a given name.
     * @param id: Buffer reference id.
     * @param name: Associated name (shader name).
     */
    virtual bool AddBufferId(EntityId id, const std::string& name) = 0;
    /**
     * @brief Check if the buffer is in the material.
     * @param id: Buffer to be checked.
     * @return True if present false otherwise.
     */
    virtual bool HasBufferId(EntityId id) const = 0;
    /**
     * @brief Remove a buffer from the material.
     * @param id: Buffer to be removed.
     * @return True if removed false otherwise.
     */
    virtual bool RemoveBufferId(EntityId id) = 0;
    /**
     * @brief Get buffer ids of a material.
     * @return Return the list of buffer ids.
     */
    virtual std::vector<EntityId> GetBufferIds() const = 0;
    /**
     * @brief Enable a texture to be used by the context.
     * @param id: Id of the texture to be enabled.
     * @return Return the name and the binding slot of a texture (to be
     * passed to the program).
     */
    virtual std::pair<std::string, int> EnableTextureId(EntityId id) const = 0;
    /**
     * @brief Unbind the texture and remove it from the list.
     * @param id: Texture id.
     */
    virtual void DisableTextureId(EntityId id) const = 0;
    /**
     * @brief Disable all the texture and unbind them.
     */
    virtual void DisableAll() const = 0;
};

} // End namespace frame.
