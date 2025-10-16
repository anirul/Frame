#pragma once

#ifndef VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <shaderc/shaderc.h>

#include "frame/json/level_data.h"
#include "frame/proto/level.pb.h"

#include "frame/camera.h"
#include "frame/device_interface.h"
#include "frame/texture_interface.h"
#include "frame/logger.h"

namespace frame::vulkan
{

/**
 * @class Device
 * @brief This is the Vulkan implementation of the device interface.
 */
class Device : public DeviceInterface
{
  public:
    Device(
        void* vk_instance,
        glm::uvec2 size,
        vk::SurfaceKHR& surface);
    ~Device();

  public:
    void SetStereo(
        StereoEnum stereo_enum,
        float interocular_distance,
        glm::vec3 focus_point,
        bool invert_left_right) final;
    void Clear(
        const glm::vec4& color = glm::vec4(.2f, 0.f, .2f, 1.0f)) const final;
    void Startup(std::unique_ptr<LevelInterface>&& level) final;
    void StartupFromLevelData(const frame::json::LevelData& level_data);
    void AddPlugin(std::unique_ptr<PluginInterface>&& plugin_interface) final;
    std::vector<PluginInterface*> GetPluginPtrs() final;
    std::vector<std::string> GetPluginNames() const final;
    void RemovePluginByName(const std::string& name) final;
    void Cleanup() final;
    void Shutdown();
    void Resize(glm::uvec2 size) final;
    glm::uvec2 GetSize() const final;
    void Display(double dt = 0.0) final;
    void ScreenShot(const std::string& file) const final;
    std::unique_ptr<BufferInterface> CreatePointBuffer(
        std::vector<float>&& vector) final;
    std::unique_ptr<BufferInterface> CreateIndexBuffer(
        std::vector<std::uint32_t>&& vector) final;
    std::unique_ptr<StaticMeshInterface> CreateStaticMesh(
        const StaticMeshParameter& static_mesh_parameter) final;

  public:
    LevelInterface& GetLevel() final
    {
        return *level_.get();
    }
    void* GetDeviceContext() const final
    {
        return vk_instance_;
    }
    StereoEnum GetStereoEnum() const
    {
        return stereo_enum_;
    }
    float GetInteroccularDistance() const
    {
        return interocular_distance_;
    }
    glm::vec3 GetFocusPoint() const
    {
        return focus_point_;
    }
    RenderingAPIEnum GetDeviceEnum() const final
    {
        return RenderingAPIEnum::VULKAN;
    }

  private:
    struct TextureResource
    {
        vk::UniqueImage image;
        vk::UniqueDeviceMemory memory;
        vk::UniqueImageView view;
        vk::UniqueSampler sampler;
        glm::uvec2 size = {0, 0};
    };

    void CreateCommandPool();
    void CreateSyncObjects();
    void CreateSwapchainResources();
    void DestroySwapchainResources();
    void RecreateSwapchain();
    void CreateGraphicsPipeline();
    void DestroyGraphicsPipeline();
    std::vector<std::uint32_t> CompileShader(
        const std::filesystem::path& path,
        shaderc_shader_kind kind) const;
    std::vector<std::uint32_t> CompileShaderSource(
        const std::string& source,
        shaderc_shader_kind kind,
        const std::string& identifier) const;
    vk::UniqueShaderModule CreateShaderModule(
        const std::vector<std::uint32_t>& code) const;
    vk::SurfaceFormatKHR SelectSurfaceFormat(
        const std::vector<vk::SurfaceFormatKHR>& formats) const;
    vk::PresentModeKHR SelectPresentMode(
        const std::vector<vk::PresentModeKHR>& modes) const;
    vk::Extent2D SelectSwapExtent(
        const vk::SurfaceCapabilitiesKHR& capabilities) const;
    void RecordCommandBuffer(
        vk::CommandBuffer command_buffer,
        std::uint32_t image_index);
    void CreateTextureResources(const frame::json::LevelData& level_data);
    void DestroyTextureResources();
    void CreateDescriptorResources();
    void DestroyDescriptorResources();
    TextureResource CreateTextureFromCpu(
        const frame::TextureInterface& texture_interface);
    struct MeshResource
    {
        vk::UniqueBuffer vertex_buffer;
        vk::UniqueDeviceMemory vertex_memory;
        vk::UniqueBuffer index_buffer;
        vk::UniqueDeviceMemory index_memory;
        std::uint32_t index_count = 0;
    };
    void CreateMeshResources(const frame::json::LevelData& level_data);
    void DestroyMeshResources();
    MeshResource CreateMeshResource(
        const frame::json::StaticMeshInfo& mesh_info);
    vk::UniqueBuffer CreateBuffer(
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties,
        vk::UniqueDeviceMemory& out_memory) const;
    void CopyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);
    void TransitionImageLayout(
        vk::Image image,
        vk::Format format,
        vk::ImageLayout old_layout,
        vk::ImageLayout new_layout);
    void CopyBufferToImage(
        vk::Buffer buffer,
        vk::Image image,
        std::uint32_t width,
        std::uint32_t height);
    std::uint32_t FindMemoryType(
        std::uint32_t type_filter,
        vk::MemoryPropertyFlags properties) const;

  private:
    std::unique_ptr<LevelInterface> level_ = nullptr;
    std::vector<std::unique_ptr<PluginInterface>> plugin_interfaces_ = {};
    vk::Instance vk_instance_ = {};
    vk::PhysicalDevice vk_physical_device_ = {};
    float queue_family_priority_ = 1.0f;
    vk::UniqueDevice vk_unique_device_;
    std::uint32_t graphics_queue_family_index_ = 0;
    std::uint32_t present_queue_family_index_ = 0;
    vk::Queue graphics_queue_;
    vk::Queue present_queue_;
    vk::SurfaceKHR& vk_surface_;
    glm::uvec2 size_ = {0, 0};
    const proto::PixelElementSize pixel_element_size_ =
        json::PixelElementSize_HALF();
    StereoEnum stereo_enum_ = StereoEnum::NONE;
    float interocular_distance_ = 0.0f;
    glm::vec3 focus_point_ = glm::vec3(0.0f);
    bool invert_left_right_ = false;
    const Logger& logger_ = Logger::GetInstance();

    vk::Format swapchain_image_format_ = vk::Format::eUndefined;
    vk::Extent2D swapchain_extent_{};
    vk::UniqueSwapchainKHR swapchain_;
    std::vector<vk::Image> swapchain_images_;
    std::vector<vk::UniqueImageView> swapchain_image_views_;
    vk::UniqueRenderPass render_pass_;
    std::vector<vk::UniqueFramebuffer> framebuffers_;
    vk::UniqueCommandPool command_pool_;
    std::vector<vk::CommandBuffer> command_buffers_;
    vk::UniquePipelineLayout pipeline_layout_;
    vk::UniquePipeline graphics_pipeline_;
    vk::UniqueDescriptorSetLayout descriptor_set_layout_;
    vk::UniqueDescriptorPool descriptor_pool_;
    vk::DescriptorSet descriptor_set_ = VK_NULL_HANDLE;
    std::vector<TextureResource> textures_;
    std::vector<MeshResource> meshes_;
    static constexpr std::size_t kMaxFramesInFlight = 2;
    std::array<vk::UniqueSemaphore, kMaxFramesInFlight> image_available_semaphores_;
    std::array<vk::UniqueSemaphore, kMaxFramesInFlight> render_finished_semaphores_;
    std::array<vk::UniqueFence, kMaxFramesInFlight> in_flight_fences_;
    std::size_t current_frame_ = 0;
    bool framebuffer_resized_ = false;
    bool sync_objects_created_ = false;
    std::optional<frame::json::LevelData> current_level_data_;
    float elapsed_time_seconds_ = 0.0f;
};

} // namespace frame::vulkan
