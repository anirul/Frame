#pragma once

#include <unordered_map>
#include <vector>

#include "frame/material_interface.h"

namespace frame::vulkan
{

class Material : public frame::MaterialInterface
{
  public:
    Material() = default;
    ~Material() override = default;

    EntityId GetProgramId(const LevelInterface* level = nullptr) const override;
    EntityId GetPreprocessProgramId(
        const LevelInterface* level = nullptr) const override;
    std::string GetInnerName(EntityId id) const override;
    void SetProgramId(EntityId id) override;
    void SetPreprocessProgramId(EntityId id) override;
    bool AddTextureId(EntityId id, const std::string& name) override;
    bool HasTextureId(EntityId id) const override;
    bool RemoveTextureId(EntityId id) override;
    std::vector<EntityId> GetTextureIds() const override;
    std::string GetInnerBufferName(const std::string& name) const override;
    bool AddBufferName(
        const std::string& name, const std::string& inner_name) override;
    std::vector<std::string> GetBufferNames() const override;
    std::string GetInnerNodeName(const std::string& name) const override;
    bool AddNodeName(
        const std::string& name, const std::string& inner_name) override;
    std::vector<std::string> GetNodeNames() const override;
    std::pair<std::string, int> EnableTextureId(EntityId id) const override;
    void DisableTextureId(EntityId id) const override;
    void DisableAll() const override
    {
        // CPU backed implementation does not bind anything.
    }

  private:
    EntityId program_id_ = NullId;
    EntityId preprocess_program_id_ = NullId;
    std::vector<EntityId> texture_order_ = {};
    std::unordered_map<EntityId, std::pair<std::string, int>> texture_map_ = {};
    std::unordered_map<std::string, std::string> buffer_map_ = {};
    std::unordered_map<std::string, std::string> node_map_ = {};
};

} // namespace frame::vulkan
