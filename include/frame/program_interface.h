#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "frame/entity_id.h"
#include "frame/json/proto.h"
#include "frame/name_interface.h"
#include "frame/uniform_collection_interface.h"

namespace frame
{

/**
 * @class Program
 * @brief This is containing the program and all associated functions.
 */
struct ProgramInterface : public NameInterface
{
    //! @brief Virtual destructor
    virtual ~ProgramInterface() = default;
    /**
     * @brief Set input texture id.
     * @param id: Add the texture id into the input program.
     */
    virtual void AddInputTextureId(EntityId id) = 0;
    /**
     * @brief Remove texture id.
     * @param id: Id to be removed from the input texture.
     */
    virtual void RemoveInputTextureId(EntityId id) = 0;
    /**
     * @brief Get input texture ids.
     * @return Vector of entity ids of input texture.
     */
    virtual std::vector<EntityId> GetInputTextureIds() const = 0;
    /**
     * @brief Set output texture id.
     * @param id: Add the texture id into the output program.
     */
    virtual void AddOutputTextureId(EntityId id) = 0;
    /**
     * @brief Remove texture id.
     * @param id: Id to be removed from the output texture.
     */
    virtual void RemoveOutputTextureId(EntityId id) = 0;
    /**
     * @brief Get output texture ids.
     * @return Vector of entity ids of output texture.
     */
    virtual std::vector<EntityId> GetOutputTextureIds() const = 0;
    /**
     * @brief Select temporary (before assignment to a entity id) scene
     *        root.
     * @return Get the temporary scene root.
     */
    virtual std::string GetTemporarySceneRoot() const = 0;
    /**
     * @brief Select temporary (before assignment to a entity id) scene
     *        root.
     * @param Set the temporary scene root.
     */
    virtual void SetTemporarySceneRoot(const std::string& name) = 0;
    /**
     * @brief Get scene root id.
     * @return Id of the scene root.
     */
    virtual EntityId GetSceneRoot() const = 0;
    /**
     * @brief Set scene root.
     * @param scene_root: Set the scene root id.
     */
    virtual void SetSceneRoot(EntityId scene_root) = 0;
    //! @brief Link shaders to a program.
    virtual void LinkShader() = 0;
    /**
     * @brief Use the program, a little bit like bind.
     * @param uniform_interface: The way to communicate the uniform like
     *        matrices (model, view, projection) but also time and other
     *        uniform that could be needed.
     */
    virtual void Use(
        const UniformCollectionInterface& uniform_collection_interface) = 0;
    /**
     * @brief Use the program, a little bit like bind.
     */
    virtual void Use() const = 0;
    //! @brief Stop using the program, a little bit like unbind.
    virtual void UnUse() const = 0;
    /**
     * @brief Get the list of uniforms needed by the program.
     * @return Vector of string that represent the names of uniforms.
     */
    virtual std::vector<std::string> GetUniformNameList() const = 0;
    /**
     * @brief Get the uniform.
     * @param name: Name of the uniform.
     * @return The uniform.
     */
    virtual const UniformInterface& GetUniform(
        const std::string& name) const = 0;
    /**
     * @brief Set the uniform.
     * @param uniform: The uniform to set.
     */
    virtual void AddUniform(std::unique_ptr<UniformInterface>&& uniform) = 0;
    /**
     * @brief Remove a uniform from the collection.
     * @param name: The name of the uniform to be removed.
     */
    virtual void RemoveUniform(const std::string& name) = 0;
    /**
     * @brief Check if the program has the uniform passed as name.
     * @param name: Name of the uniform.
     * @return True if present false otherwise.
     */
    virtual bool HasUniform(const std::string& name) const = 0;
    /**
     * @brief Add uniform enum.
     * @param name: Uniform name.
     * @param uniform_enum: Enum associated to the uniform.
     */
    virtual void AddUniformEnum(
        const std::string& name, proto::Uniform::UniformEnum uniform_enum) = 0;
};

} // End namespace frame.
