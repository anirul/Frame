#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

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
class CommandQueue;
class BufferResourceManager;
class GpuMemoryManager;
class OutputImageResources;
class PipelineResources;
class RaytraceSceneRenderer;
class Renderer;
class ShaderCompiler;
class SwapchainResources;
class SyncResources;
class Texture;
class TextureResources;
struct SceneState;
struct RaytracingSourceGeometryData
{
    EntityId source_node_id = NullId;
    EntityId triangle_buffer_id = NullId;
    EntityId source_material_id = NullId;
    std::uint32_t material_id = 0;
    std::uint32_t triangle_offset = 0;
    std::vector<std::uint8_t> triangle_bytes = {};
};

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
    std::unique_ptr<MeshInterface> CreateMesh(
        const MeshParameter& mesh_parameter) final;

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
    friend class OutputImageResources;
    friend class PipelineResources;
    friend class RaytraceSceneRenderer;
    friend class Renderer;
    void CreateGraphicsPipeline();
    void DestroyGraphicsPipeline();
    void CreateComputePipeline();
    void DestroyComputePipeline();
    void CreateRaytracingPipeline();
    void DestroyRaytracingPipeline();
    void CreateHardwareRaytracingScene();
    void DestroyHardwareRaytracingScene();
    void CreateComputeOutputImage();
    void DestroyComputeOutputImage();
    void CreateSwapchainPreviewImage();
    void DestroySwapchainPreviewImage();
    void RecreateSwapchain();
    void LogRuntimeConfiguration() const;
    SceneState BuildFrameSceneState(vk::Extent2D extent) const;
    void CreateTextureResources(const frame::json::LevelData& level_data);
    void DestroyTextureResources();
    void CreateDescriptorResources();
    void DestroyDescriptorResources();
    void CreateGpuSkinningResources();
    void DestroyGpuSkinningResources();
    void UpdateRaytraceBuffers();
    std::vector<EntityId> UpdateGpuSkinnedMeshes();
    bool UpdateAggregateRaytracingSceneBuffers(bool build_software_bvh);
    bool UpdateHardwareRaytracingAggregateSceneBuffers(
        const std::vector<EntityId>& updated_source_triangle_buffer_ids);
    bool UpdateHardwareRaytracingAggregateSceneBuffers(
        const std::vector<EntityId>& updated_source_triangle_buffer_ids,
        const std::vector<RaytracingSourceGeometryData>& prepared_source_geometries);
    void UpdateHardwareRaytracingScene();
    bool UpdateHardwareRaytracingDynamicGeometry();
    bool UpdateHardwareRaytracingDynamicGeometry(
        const std::vector<EntityId>& updated_source_triangle_buffer_ids);
    bool UpdateHardwareRaytracingDynamicGeometry(
        const std::vector<EntityId>& updated_source_triangle_buffer_ids,
        const std::vector<RaytracingSourceGeometryData>& prepared_source_geometries);
    bool UpdateHardwareRaytracingTransforms(bool force_tlas_update = false);
    void RebuildHardwareRaytracingTlas();
    void UpdateHardwareRaytracingDescriptor();
    bool UpdateHardwareRaytracingInstanceStorageBuffer();
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
    std::unique_ptr<PipelineResources> pipeline_resources_;
    std::unique_ptr<OutputImageResources> output_image_resources_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<ShaderCompiler> shader_compiler_;
    std::unique_ptr<GpuMemoryManager> gpu_memory_manager_;
    std::unique_ptr<CommandQueue> command_queue_;
    std::unique_ptr<BufferResourceManager> buffer_resources_;
    std::unique_ptr<class MeshResources> mesh_resources_;
    vk::UniqueDescriptorSetLayout descriptor_set_layout_;
    vk::UniqueDescriptorPool descriptor_pool_;
    vk::DescriptorSet descriptor_set_ = VK_NULL_HANDLE;
    std::unique_ptr<TextureResources> texture_resources_;
    static constexpr std::size_t kMaxFramesInFlight = 2;
    bool framebuffer_resized_ = false;
    bool use_compute_raytracing_ = false;
    bool use_raytracing_pipeline_ = false;
    bool storage_buffers_ready_ = false;
    vk::Format compute_output_format_ = vk::Format::eR16G16B16A16Sfloat;
    bool device_lost_ = false;
    bool hardware_raytracing_supported_ = false;
    bool use_hardware_raytracing_ = false;
    struct GpuSkinningResource
    {
        EntityId mesh_id = NullId;
        EntityId source_triangle_buffer_id = NullId;
        EntityId source_node_id = NullId;
        EntityId source_material_id = NullId;
        std::uint32_t material_id = 0;
        std::uint32_t triangle_offset = 0;
        std::uint32_t output_vertex_count = 0;
        std::uint32_t bone_capacity = 0;
        vk::DeviceSize output_buffer_size = 0;
        glm::vec4 color_multiplier = glm::vec4(1.0f);
        vk::UniqueBuffer source_vertex_buffer;
        vk::UniqueDeviceMemory source_vertex_memory;
        vk::UniqueBuffer source_index_buffer;
        vk::UniqueDeviceMemory source_index_memory;
        vk::UniqueBuffer bone_matrix_buffer;
        vk::UniqueDeviceMemory bone_matrix_memory;
        vk::UniqueBuffer output_buffer;
        vk::UniqueDeviceMemory output_memory;
        vk::DescriptorSet descriptor_set = VK_NULL_HANDLE;
        bool initialized = false;
    };
    std::vector<GpuSkinningResource> gpu_skinning_resources_;
    vk::UniqueDescriptorSetLayout gpu_skinning_descriptor_set_layout_;
    vk::UniqueDescriptorPool gpu_skinning_descriptor_pool_;
    vk::UniquePipelineLayout gpu_skinning_pipeline_layout_;
    vk::UniquePipeline gpu_skinning_pipeline_;
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
        std::filesystem::path raygen_shader;
        std::filesystem::path miss_shader;
        std::filesystem::path closesthit_shader;
        frame::proto::SceneType::Enum scene_type = frame::proto::SceneType::NONE;
        bool uses_time_uniform = false;
        bool use_compute = false;
        std::vector<BindingInfo> bindings;
        std::unordered_map<std::string, EntityId> texture_ids_by_inner;
        std::unordered_map<std::string, EntityId> buffer_ids_by_inner;
    };

    std::optional<frame::json::LevelData> current_level_data_;
    std::optional<ProgramPipelineInfo> active_program_info_;
    float elapsed_time_seconds_ = 0.0f;
    std::size_t last_raytrace_scene_state_hash_ = 0;
    bool has_raytrace_scene_state_hash_ = false;
    GuiRenderCallback gui_render_callback_;
    vk::PhysicalDeviceBufferDeviceAddressFeatures
        buffer_device_address_features_ = {};
    vk::PhysicalDeviceAccelerationStructureFeaturesKHR
        acceleration_structure_features_ = {};
    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR
        raytracing_pipeline_features_ = {};
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR
        raytracing_pipeline_properties_ = {};
    struct HardwareRaytracingGeometry
    {
        std::string source_inner_name;
        EntityId source_node_id = NullId;
        EntityId source_buffer_id = NullId;
        vk::UniqueBuffer vertex_buffer;
        vk::UniqueDeviceMemory vertex_memory;
        vk::DeviceSize vertex_buffer_size = 0;
        std::uint32_t vertex_count = 0;
        vk::UniqueBuffer index_buffer;
        vk::UniqueDeviceMemory index_memory;
        vk::DeviceSize index_buffer_size = 0;
        vk::UniqueBuffer blas_buffer;
        vk::UniqueDeviceMemory blas_memory;
        vk::UniqueAccelerationStructureKHR blas;
        vk::DeviceAddress blas_address = 0;
        std::uint32_t triangle_count = 0;
        std::uint32_t instance_custom_index = 0;
        std::uint32_t material_id = 0;
        std::uint32_t triangle_offset = 0;
        EntityId source_material_id = NullId;
    };
    std::vector<HardwareRaytracingGeometry> hardware_raytracing_geometries_;
    bool hardware_raytracing_uses_source_instances_ = false;
    vk::UniqueBuffer hardware_raytracing_instance_buffer_;
    vk::UniqueDeviceMemory hardware_raytracing_instance_memory_;
    std::vector<std::uint8_t> hardware_raytracing_instance_bytes_;
    vk::UniqueBuffer hardware_raytracing_tlas_buffer_;
    vk::UniqueDeviceMemory hardware_raytracing_tlas_memory_;
    vk::UniqueAccelerationStructureKHR hardware_raytracing_tlas_;
};

} // namespace frame::vulkan


