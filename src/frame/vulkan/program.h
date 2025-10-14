#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "frame/program_interface.h"
#include "frame/uniform_interface.h"

namespace frame::vulkan
{

class Program : public frame::ProgramInterface
{
  public:
    explicit Program(const std::string& name);
    ~Program() override = default;

    void AddInputTextureId(EntityId id) override;
    void RemoveInputTextureId(EntityId id) override;
    std::vector<EntityId> GetInputTextureIds() const override;
    void AddOutputTextureId(EntityId id) override;
    void RemoveOutputTextureId(EntityId id) override;
    std::vector<EntityId> GetOutputTextureIds() const override;
    std::string GetTemporarySceneRoot() const override;
    void SetTemporarySceneRoot(const std::string& name) override;
    EntityId GetSceneRoot() const override;
    void SetSceneRoot(EntityId scene_root) override;
    void LinkShader() override {}
    void Use(
        const UniformCollectionInterface& uniform_collection_interface,
        const LevelInterface* level = nullptr) override;
    void Use() const override {}
    void UnUse() const override {}
    std::vector<std::string> GetUniformNameList() const override;
    const UniformInterface& GetUniform(const std::string& name) const override;
    void AddUniform(std::unique_ptr<UniformInterface>&& uniform) override;
    void RemoveUniform(const std::string& name) override;
    bool HasUniform(const std::string& name) const override;

  private:
    std::vector<EntityId> input_textures_ = {};
    std::vector<EntityId> output_textures_ = {};
    std::string temporary_scene_root_;
    EntityId scene_root_ = NullId;
    std::map<std::string, std::unique_ptr<UniformInterface>> uniforms_;
    bool is_used_ = false;
};

} // namespace frame::vulkan
