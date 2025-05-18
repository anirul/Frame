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
#include "frame/uniform.h"
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
     * @brief Use the program, a little bit like bind.
     * @param uniform_interface: The way to communicate the uniform like
     * matrices (model, view, projection) but also time and other uniform
     * that could be needed.
     */
    void Use(const UniformCollectionInterface& uniform_collection_interface)
        override;
    /**
     * @brief Use the program, a little bit like bind.
     */
    void Use() const override;
    //! @brief Stop using the program, a little bit like unbind.
    void UnUse() const override;

  protected:
    /**
     * @brief Get the memoize version of the uniform (stored locally).
     * @param name: Name of the uniform.
     * @return Id of the uniform.
     */
    int GetMemoizeUniformLocation(const std::string& name) const;
    /**
     * @brief Throw an exception in case this texture is already in the
     * program.
     * @param texture_id: Texture id to be tested.
     */
    void ThrowIsInTextureIds(EntityId texture_id) const;
    /**
     * @brief Create the uniform value list (internal).
     */
    void CreateUniformList();

  private:
    const Logger& logger_ = Logger::GetInstance();
    mutable std::map<std::string, int> memoize_map_ = {};
    mutable std::map<std::string, std::unique_ptr<UniformInterface>>
        uniform_map_ = {};
    std::vector<unsigned int> attached_shaders_ = {};
    std::string temporary_scene_root_;
    std::string name_;
    int program_id_ = 0;
    EntityId scene_root_ = 0;
    std::vector<EntityId> input_texture_ids_ = {};
    std::vector<EntityId> output_texture_ids_ = {};
    std::map<std::string, proto::Uniform::UniformEnum> name_uniform_enums_;
	std::string shader_name_;
	bool serialize_enable_ = false;
    mutable bool is_used_ = false;
};

/**
 * @brief Create a program from two streams.
 * @param name: Name of the uniform.
 * @param value: Enum value of the uniform (see proto declaration for that).
 */
std::unique_ptr<frame::ProgramInterface> CreateProgram(
    const std::string& name,
    const std::string& shader_name,
    std::istream& vertex_shader_code,
    std::istream& pixel_shader_code);
/**
 * @brief Create a program from two streams.
 * @param name: Name of the uniform.
 * @param value: Enum value of the uniform (see proto declaration for that).
 */
std::unique_ptr<frame::ProgramInterface> CreateProgram(
    const proto::Program& proto_program,
    std::istream& vertex_shader_code,
    std::istream& pixel_shader_code);

} // End namespace frame::opengl.
