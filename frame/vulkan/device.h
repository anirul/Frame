#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "frame/json/level_data.h"
#include "frame/proto/level.pb.h"

#include "frame/camera.h"
#include "frame/device_interface.h"
#include "frame/texture_interface.h"
#include "frame/logger.h"
#include "frame/vulkan/buffer_resources.h"
#include "frame/vulkan/mesh_resources.h"
#include "frame/vulkan/vulkan_dispatch.h"
 
namespace frame::vulkan
{

class CommandResources;
class ShaderCompiler;
class SwapchainResources;
class SyncResources;
class Texture;
class TextureResources;

/**
 * @class Device
 * @brief This is the Vulkan implementation of the device interface.
 */
class Device : public DeviceInterface
{
  public:
    using GuiRenderCallback = std::function<void(vk::CommandBuffer)>;

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
    void SetGuiRenderCallback(GuiRenderCallback callback);
    void ClearGuiRenderCallback();
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
    vk::Instance GetVkInstance() const
    {
        return vk_instance_;
    }
    vk::PhysicalDevice GetVkPhysicalDevice() const
    {
        return vk_physical_device_;
    }
    vk::Device GetVkDevice() const
    {
        return vk_unique_device_ ? vk_unique_device_.get() : vk::Device{};
    }
    vk::Queue GetGraphicsQueue() const
    {
        return graphics_queue_;
    }
    std::uint32_t GetGraphicsQueueFamilyIndex() const
    {
        return graphics_queue_family_index_;
    }
    std::uint32_t GetMaxFramesInFlight() const
    {
        return static_cast<std::uint32_t>(kMaxFramesInFlight);
    }
    const SwapchainResources* GetSwapchainResources() const
    {
        return swapchain_resources_.get();
    }
    const CommandResources* GetCommandResources() const
    {
        return command_resources_.get();
    }
    std::optional<vk::DescriptorImageInfo> GetComputeOutputDescriptorInfo() const;
    std::optional<vk::DescriptorImageInfo> GetSwapchainPreviewDescriptorInfo() const;

  private:
    friend class TextureResources;
    void CreateGraphicsPipeline();
    void DestroyGraphicsPipeline();
    void CreateComputePipeline();
    void DestroyComputePipeline();
    void CreateComputeOutputImage();
    void DestroyComputeOutputImage();
    void CreateSwapchainPreviewImage();
    void DestroySwapchainPreviewImage();
    void RecreateSwapchain();
    vk::UniqueShaderModule CreateShaderModule(
        const std::vector<std::uint32_t>& code) const;
    void RecordCommandBuffer(
        vk::CommandBuffer command_buffer,
        std::uint32_t image_index);
    void CreateTextureResources(const frame::json::LevelData& level_data);
    void DestroyTextureResources();
    void CreateDescriptorResources();
    void DestroyDescriptorResources();
    void CopyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);
    void TransitionImageLayout(
        vk::Image image,
        vk::Format format,
        vk::ImageLayout old_layout,
        vk::ImageLayout new_layout,
        std::uint32_t layer_count = 1);
    void CopyBufferToImage(
        vk::Buffer buffer,
        vk::Image image,
        std::uint32_t width,
        std::uint32_t height,
        std::uint32_t layer_count = 1,
        std::size_t layer_stride = 0);

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

    std::unique_ptr<SwapchainResources> swapchain_resources_;
    std::unique_ptr<CommandResources> command_resources_;
    std::unique_ptr<SyncResources> sync_resources_;
    std::unique_ptr<ShaderCompiler> shader_compiler_;
    vk::UniquePipelineLayout pipeline_layout_;
    vk::UniquePipeline graphics_pipeline_;
    vk::UniquePipeline compute_pipeline_;
    vk::UniquePipelineLayout compute_pipeline_layout_;
    std::unique_ptr<class GpuMemoryManager> gpu_memory_manager_;
    std::unique_ptr<class CommandQueue> command_queue_;
    std::unique_ptr<class BufferResourceManager> buffer_resources_;
    std::unique_ptr<class MeshResources> mesh_resources_;
    vk::UniqueDescriptorSetLayout descriptor_set_layout_;
    vk::UniqueDescriptorPool descriptor_pool_;
    vk::DescriptorSet descriptor_set_ = VK_NULL_HANDLE;
    std::unique_ptr<TextureResources> texture_resources_;
    static constexpr std::size_t kMaxFramesInFlight = 2;
    std::size_t current_frame_ = 0;
    bool framebuffer_resized_ = false;
    bool use_compute_raytracing_ = false;
    vk::UniqueImage compute_output_image_;
    vk::UniqueDeviceMemory compute_output_memory_;
    vk::UniqueImageView compute_output_view_;
    vk::UniqueSampler compute_output_sampler_;
    bool compute_output_in_shader_read_ = false;
    vk::UniqueImage swapchain_preview_image_;
    vk::UniqueDeviceMemory swapchain_preview_memory_;
    vk::UniqueImageView swapchain_preview_view_;
    vk::UniqueSampler swapchain_preview_sampler_;
    bool swapchain_preview_in_shader_read_ = false;
    vk::Format swapchain_preview_format_ = vk::Format::eUndefined;
    glm::uvec2 swapchain_preview_size_ = {0, 0};
    bool storage_buffers_ready_ = false;
    vk::Format compute_output_format_ = vk::Format::eR16G16B16A16Sfloat;
    bool device_lost_ = false;
    struct ProgramPipelineInfo
    {
        struct BindingInfo
        {
            std::string name;
            std::uint32_t binding = 0;
            frame::proto::ProgramBinding::BindingType binding_type =
                frame::proto::ProgramBinding::BINDING_INVALID;
            vk::ShaderStageFlags stages = {};
        };

        std::string program_name;
        EntityId program_id = NullId;
        EntityId material_id = NullId;
        std::filesystem::path vertex_shader;
        std::filesystem::path fragment_shader;
        std::filesystem::path compute_shader;
        frame::proto::SceneType::Enum scene_type = frame::proto::SceneType::NONE;
        bool uses_time_uniform = false;
        bool use_compute = false;
        std::vector<BindingInfo> bindings;
        std::unordered_map<std::string, EntityId> texture_ids_by_inner;
        std::unordered_map<std::string, EntityId> buffer_ids_by_inner;
    };

    std::optional<frame::json::LevelData> current_level_data_;
    std::optional<ProgramPipelineInfo> active_program_info_;
    bool use_procedural_quad_pipeline_ = false;
    float elapsed_time_seconds_ = 0.0f;
    vk::ShaderStageFlags push_constant_stages_ = {};
    std::uint32_t push_constant_size_ = 0;
    GuiRenderCallback gui_render_callback_;
};

} // namespace frame::vulkan
