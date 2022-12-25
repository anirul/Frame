#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "frame/entity_id.h"
#include "frame/json/proto.h"
#include "frame/name_interface.h"
#include "frame/uniform_interface.h"

namespace frame {

/**
 * @class Program
 * @brief This is containing the program and all associated functions.
 */
struct ProgramInterface : public NameInterface {
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
     * @brief Select temporary (before assignment to a entity id) scene root.
     * @return Get the temporary scene root.
     */
    virtual std::string GetTemporarySceneRoot() const = 0;
    /**
     * @brief Select temporary (before assignment to a entity id) scene root.
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
     * @param uniform_interface: The way to communicate the uniform like matrices (model, view,
     * projection) but also time and other uniform that could be needed.
     */
    virtual void Use(const UniformInterface* uniform_interface) const = 0;
    //! @brief Stop using the program, a little bit like unbind.
    virtual void UnUse() const = 0;
    /**
     * @brief Get the list of uniforms needed by the program.
     * @return Vector of string that represent the names of uniforms.
     */
    virtual std::vector<std::string> GetUniformNameList() const = 0;
    /**
     * @brief Create a uniform from a string and a bool.
     * @param name: Name of the uniform.
     * @param value: Boolean.
     */
    virtual void Uniform(const std::string& name, bool value) const = 0;
    /**
     * @brief Create a uniform from a string and an int.
     * @param name: Name of the uniform.
     * @param value: Integer.
     */
    virtual void Uniform(const std::string& name, int value) const = 0;
    /**
     * @brief Create a uniform from a string and a float.
     * @param name: Name of the uniform.
     * @param value: Float.
     */
    virtual void Uniform(const std::string& name, float value) const = 0;
    /**
     * @brief Create a uniform from a string and a vector2.
     * @param name: Name of the uniform.
     * @param value: Vector2.
     */
    virtual void Uniform(const std::string& name, const glm::vec2 vec2) const = 0;
    /**
     * @brief Create a uniform from a string and a vector3.
     * @param name: Name of the uniform.
     * @param value: Vector3.
     */
    virtual void Uniform(const std::string& name, const glm::vec3 vec3) const = 0;
    /**
     * @brief Create a uniform from a string and a vector4.
     * @param name: Name of the uniform.
     * @param value: Vector4.
     */
    virtual void Uniform(const std::string& name, const glm::vec4 vec4) const = 0;
    /**
     * @brief Create a uniform from a string and a matrix.
     * @param name: Name of the uniform.
     * @param value: Matrix.
     */
    virtual void Uniform(const std::string& name, const glm::mat4 mat) const = 0;
    /**
     * @brief Create a uniform from a string and a vector.
     * @param name: Name of the uniform.
     * @param vector: Vector to be inputed into the uniform.
     */
    virtual void Uniform(const std::string& name, const std::vector<float>& vector,
                         glm::uvec2 size = { 0, 0 }) const = 0;
    /**
     * @brief Create a uniform from a string and a vector.
     * @param name: Name of the uniform.
     * @param vector: Vector to be inputed into the uniform.
     */
    virtual void Uniform(const std::string& name, const std::vector<std::int32_t>& vector,
                         glm::uvec2 size = { 0, 0 }) const = 0;
    /**
     * @brief Check if the program has the uniform passed as name.
     * @param name: Name of the uniform.
     * @return True if present false otherwise.
     */
    virtual bool HasUniform(const std::string& name) const = 0;
};

}  // End namespace frame.
