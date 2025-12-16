#pragma once

#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "frame/json/level_data.h"
#include "frame/level_interface.h"
#include "frame/vulkan/texture.h"

namespace frame::vulkan
{

class Device;

class TextureResources
{
  public:
    explicit TextureResources(Device& owner);

    void Build(LevelInterface& level, const frame::json::LevelData& level_data);
    void Destroy();
    bool CollectDescriptorInfos(
        const std::vector<EntityId>& requested_ids,
        std::vector<vk::DescriptorImageInfo>& out_infos,
        std::vector<EntityId>& out_ids) const;
    bool Empty() const
    {
        return textures_.empty();
    }

  private:
    void UploadTexture(EntityId id, frame::vulkan::Texture& texture);

    Device& owner_;
    std::unordered_map<EntityId, frame::vulkan::Texture*> textures_;
};

} // namespace frame::vulkan
