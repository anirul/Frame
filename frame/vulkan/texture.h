#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "frame/vulkan/vulkan_dispatch.h"

#include "frame/texture_interface.h"

namespace frame::vulkan
{

class Texture : public frame::TextureInterface
{
  public:
    Texture(const frame::proto::Texture& proto, glm::uvec2 display_size);

    void Clear(const glm::vec4 color) override;
    std::vector<std::uint8_t> GetTextureByte() const override;
    std::vector<std::uint16_t> GetTextureWord() const override;
    std::vector<std::uint32_t> GetTextureDWord() const override;
    std::vector<float> GetTextureFloat() const override;
    const std::vector<std::uint8_t>& GetTextureData() const
    {
        return data_;
    }
    void Update(
        std::vector<std::uint8_t>&& vector,
        glm::uvec2 size,
        std::uint8_t bytes_per_pixel) override;
    void EnableMipmap() override
    {
        mipmap_enabled_ = true;
    }
    glm::uvec2 GetSize() override
    {
        return size_;
    }
    void SetDisplaySize(glm::uvec2 display_size) override
    {
        display_size_ = display_size;
    }
    void SetViewType(vk::ImageViewType view_type)
    {
        view_type_ = view_type;
    }
    vk::ImageViewType GetViewType() const
    {
        return view_type_;
    }

    bool HasGpuResources() const;
    void SetGpuResources(
        vk::Format format,
        vk::ImageViewType view_type,
        vk::UniqueImage image,
        vk::UniqueDeviceMemory memory,
        vk::UniqueImageView view,
        vk::UniqueSampler sampler);
    bool MoveGpuResourcesTo(Texture& target);
    void ResetGpuResources();
    vk::DescriptorImageInfo GetDescriptorInfo() const;

    static std::uint8_t BytesPerComponent(proto::PixelElementSize::Enum value);
    static std::uint8_t ComponentCount(proto::PixelStructure::Enum value);
  private:

    void SyncProtoSize();
    void SyncProtoPixels();

    glm::uvec2 size_{0, 0};
    glm::uvec2 display_size_{0, 0};
    std::vector<std::uint8_t> data_{};
    std::uint8_t bytes_per_pixel_ = 4;
    bool mipmap_enabled_ = false;
    vk::Format gpu_format_ = vk::Format::eUndefined;
    vk::ImageViewType view_type_ = vk::ImageViewType::e2D;
    vk::UniqueImage image_;
    vk::UniqueDeviceMemory memory_;
    vk::UniqueImageView view_;
    vk::UniqueSampler sampler_;
};

} // namespace frame::vulkan
