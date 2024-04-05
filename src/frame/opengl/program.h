#pragma once

#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "frame/json/proto.h"
#include "frame/logger.h"
#include "frame/opengl/shader.h"
#include "frame/program_interface.h"
#include "frame/uniform_interface.h"

namespace frame::opengl
{

/**
 * @class Program
 * @brief This is containing the program and all associated functions.
 */
class Program : public ProgramInterface
{
  public:
    //! @brief Constructor create the program.
    Program(const std::string& name);
    //! @brief Destructor destroy objects.
    virtual ~Program();

  public:
    /**
     * @brief Get name from the name interface.
     * @return The name of the object.
     */
    std::string GetName() const override
    {
        return name_;
    }
    /**
     * @brief Set name from the name interface.
     * @param name: New name to be set.
     */
    void SetName(const std::string& name) override
    {
        name_ = name;
    }

  public:
    /**
     * @brief Set input texture id.
     * @param id: Add the texture id into the input program.
     */
    void AddInputTextureId(EntityId id) override;
    /**
     * @brief Remove texture id.
     * @param id: Id to be removed from the input texture.
     */
    void RemoveInputTextureId(EntityId id) override;
    /**
     * @brief Get input texture ids.
     * @return Vector of entity ids of input texture.
     */
    std::vector<EntityId> GetInputTextureIds() const override;
    /**
     * @brief Set output texture id.
     * @param id: Add the texture id into the output program.
     */
    void AddOutputTextureId(EntityId id) override;
    /**
     * @brief Remove texture id.
     * @param id: Id to be removed from the output texture.
     */
    void RemoveOutputTextureId(EntityId id) override;
    /**
     * @brief Get output texture ids.
     * @return Vector of entity ids of output texture.
     */
    std::vector<EntityId> GetOutputTextureIds() const override;
    /**
     * @brief Select temporary (before assignment to a entity id) scene
     * root.
     * @return Get the temporary scene root.
     */
    std::string GetTemporarySceneRoot() const override;
    /**
     * @brief Select temporary (before assignment to a entity id) scene
     * root.
     * @param Set the temporary scene root.
     */
    void SetTemporarySceneRoot(const std::string& name) override;
    /**
     * @brief Get scene root id.
     * @return Id of the scene root.
     */
    EntityId GetSceneRoot() const override;
    /**
     * @brief Set scene root.
     * @param scene_root: Set the scene root id.
     */
    void SetSceneRoot(EntityId scene_root) override;
    /**
     * @brief Attach shader to a program.
     * @param shader: Add a shader to the program.
     */
    void AddShader(const Shader& shader);
    //! @brief Link shaders to a program.
    void LinkShader() override;
    /**
     * @brief Get the list of uniforms needed by the program.
     * @return Vector of string that represent the names of uniforms.
     */
    std::vector<std::string> GetUniformNameList() const override;
    /**
     * @brief Use the program, a little bit like bind.
     * @param uniform_interface: The way to communicate the uniform like
     * matrices (model, view, projection) but also time and other uniform
     * that could be needed.
     */
    void Use(const UniformInterface& uniform_interface) const override;
    /**
     * @brief Use the program, a little bit like bind.
     */
    void Use() const override;
    //! @brief Stop using the program, a little bit like unbind.
    void UnUse() const override;
    /**
     * @brief Create a uniform from a string and a bool.
     * @param name: Name of the uniform.
     * @param value: Boolean.
     */
    void Uniform(const std::string& name, bool value) const override;
    /**
     * @brief Create a uniform from a string and an int.
     * @param name: Name of the uniform.
     * @param value: Integer.
     */
    void Uniform(const std::string& name, int value) const override;
    /**
     * @brief Create a uniform from a string and a float.
     * @param name: Name of the uniform.
     * @param value: Float.
     */
    void Uniform(const std::string& name, float value) const override;
    /**
     * @brief Create a uniform from a string and a vector2.
     * @param name: Name of the uniform.
     * @param value: Vector2.
     */
    void Uniform(const std::string& name, const glm::vec2 vec2) const override;
    /**
     * @brief Create a uniform from a string and a vector3.
     * @param name: Name of the uniform.
     * @param value: Vector3.
     */
    void Uniform(const std::string& name, const glm::vec3 vec3) const override;
    /**
     * @brief Create a uniform from a string and a vector4.
     * @param name: Name of the uniform.
     * @param value: Vector4.
     */
    void Uniform(const std::string& name, const glm::vec4 vec4) const override;
    /**
     * @brief Create a uniform from a string and a matrix.
     * @param name: Name of the uniform.
     * @param value: Matrix.
     */
    void Uniform(const std::string& name, const glm::mat4 mat) const override;
    /**
     * @brief Create a uniform from a string and a vector.
     * @param name: Name of the uniform.
     * @param vector: Vector to be inputed into the uniform.
     */
    void Uniform(
        const std::string& name,
        const std::vector<glm::vec2>& vector) const override;
    /**
     * @brief Create a uniform from a string and a vector.
     * @param name: Name of the uniform.
     * @param vector: Vector to be inputed into the uniform.
     */
    void Uniform(
        const std::string& name,
        const std::vector<glm::vec3>& vector) const override;
    /**
     * @brief Create a uniform from a string and a vector.
     * @param name: Name of the uniform.
     * @param vector: Vector to be inputed into the uniform.
     */
    void Uniform(
        const std::string& name,
        const std::vector<glm::vec4>& vector) const override;
    /**
     * @brief Create a uniform from a string and a vector.
     * For now this is checking the size of the vector to input in the
     * corresponding matrix.
     * @param name: Name of the uniform.
     * @param vector: Vector to be inputed into the uniform.
     * @param size: Size of the vector (ex: 3x3 for a mat3).
     */
    void Uniform(
        const std::string& name,
        const std::vector<float>& vector,
        glm::uvec2 size = {0, 0}) const override;
    /**
     * @brief Create a uniform from a string and a vector.
     * For now this is checking the size of the vector to input in the
     * corresponding matrix.
     * @param name: Name of the uniform.
     * @param vector: Vector to be inputed into the uniform.
     * @param size: Size of the vector (ex: 3x3 for a mat3).
     */
    void Uniform(
        const std::string& name,
        const std::vector<std::int32_t>& vector,
        glm::uvec2 size = {0, 0}) const override;
    /**
     * @brief Check if the program has the uniform passed as name.
     * @param name: Name of the uniform.
     * @return True if present false otherwise.
     */
    bool HasUniform(const std::string& name) const override;

  protected:
    /**
     * @brief Get the memoize version of the uniform (stored locally).
     * @param name: Name of the uniform.
     * @return Id of the uniform.
     */
    int GetMemoizeUniformLocation(const std::string& name) const;
    /**
     * @brief Test if the uniform is in the uniform list.
     * @param name: Uniform to be tested.
     * @return True if the uniform is in the list.
     */
    bool IsUniformInList(const std::string& name) const;
    /**
     * @brief Throw an exception in case this texture is already in the
     * program.
     * @param texture_id: Texture id to be tested.
     */
    void ThrowIsInTextureIds(EntityId texture_id) const;
    /**
     * @brief Create the uniform value list (internal).
     */
    void CreateUniformList() const;

  private:
    /**
     * @brief value of a uniform.
     */
    struct UniformValue
    {
        GLsizei length;
        GLsizei size;
        GLenum type;
        std::string name;
    };
    const Logger& logger_ = Logger::GetInstance();
    mutable std::map<std::string, int> memoize_map_ = {};
    mutable std::map<std::string, proto::Uniform::UniformEnum>
        uniform_float_variable_map_ = {};
    mutable std::map<std::string, proto::Uniform::UniformEnum>
        uniform_int_variable_map_ = {};
    mutable std::vector<UniformValue> uniform_list_ = {};
    std::vector<unsigned int> attached_shaders_ = {};
    std::string temporary_scene_root_;
    std::string name_;
    int program_id_ = 0;
    EntityId scene_root_ = 0;
    std::vector<EntityId> input_texture_ids_ = {};
    std::vector<EntityId> output_texture_ids_ = {};
};

/**
 * @brief Create a program from two streams.
 * @param name: Name of the uniform.
 * @param value: Enum value of the uniform (see proto declaration for that).
 */
std::unique_ptr<frame::ProgramInterface> CreateProgram(
    const std::string& name,
    std::istream& vertex_shader_code,
    std::istream& pixel_shader_code);

} // End namespace frame::opengl.
