#include "frame/vulkan/device.h"

namespace frame::vulkan
{

Device::Device(glm::uvec2 size) : size_(size)
{
    // Log the Vulkan version.
    logger_->info("Creating Vulkan Device ({}, {})", size.x, size.y);
    auto api_version = vk_context_.enumerateInstanceVersion();
    logger_->info(
        "Vulkan version: {}.{} total: {}",
        VK_VERSION_MAJOR(api_version),
        VK_VERSION_MINOR(api_version),
        api_version);
    // List all available extensions.
    std::vector<vk::ExtensionProperties> availableExtensions =
        vk::enumerateInstanceExtensionProperties();
    logger_->info("Available Extensions:");
    for (const auto& extension : availableExtensions)
    {
        std::string extension_name = extension.extensionName;
        logger_->info("\t[{}]", extension_name);
    }
    // List all available layers.
    std::vector<vk::LayerProperties> availableLayers =
        vk::enumerateInstanceLayerProperties();
    logger_->info("Available Layers:");
    for (const auto& layer : availableLayers)
    {
        std::string layer_name = layer.layerName;
        logger_->info("\t[{}]", layer_name);
    }
}

Device::~Device()
{
    logger_->info("Destroying Vulkan Device");
}

void Device::SetStereo(
    StereoEnum stereo_enum,
    float interocular_distance,
    glm::vec3 focus_point,
    bool invert_left_right)
{
    throw std::runtime_error("Not implemented!");
}

void Device::Init(vk::InstanceCreateInfo instance_create_info)
{
    logger_->info("Initializing Vulkan Device");
    // Create the instance.
    vk_instance_.emplace(vk_context_, instance_create_info);
    // Get all vk::PhysicalDeviceGroupProperties from a vk::raii::Instance
    // instance.
    std::vector<vk::PhysicalDeviceGroupProperties>
        physical_device_group_properties =
            vk_instance_.value().enumeratePhysicalDeviceGroups();
    logger_->info(
        "Number of physical devices: {}",
        physical_device_group_properties.size());
    for (const auto& physical_device_group : physical_device_group_properties)
    {
        logger_->info(
            "\t- group({})", physical_device_group.physicalDeviceCount);
        for (std::uint32_t i = 0; i < physical_device_group.physicalDeviceCount;
             ++i)
        {
            std::string device_name = physical_device_group.physicalDevices[i]
                                          .getProperties()
                                          .deviceName;
            logger_->info("\t\t- {}", device_name);
        }
    }
}

void Device::Clear(
    const glm::vec4& color /*= glm::vec4(.2f, 0.f, .2f, 1.0f)*/) const
{
    throw std::runtime_error("Not implemented!");
}

void Device::Startup(std::unique_ptr<LevelInterface>&& level)
{
    throw std::runtime_error("Not implemented!");
}

void Device::AddPlugin(std::unique_ptr<PluginInterface>&& plugin_interface)
{
    throw std::runtime_error("Not implemented!");
}

std::vector<PluginInterface*> Device::GetPluginPtrs()
{
    throw std::runtime_error("Not implemented!");
}

std::vector<std::string> Device::GetPluginNames() const
{
    throw std::runtime_error("Not implemented!");
}

void Device::RemovePluginByName(const std::string& name)
{
    throw std::runtime_error("Not implemented!");
}

void Device::Cleanup()
{
    throw std::runtime_error("Not implemented!");
}

void Device::Resize(glm::uvec2 size)
{
    throw std::runtime_error("Not implemented!");
}

glm::uvec2 Device::GetSize() const
{
    throw std::runtime_error("Not implemented!");
}

void Device::Display(double dt /*= 0.0*/)
{
    throw std::runtime_error("Not implemented!");
}

void Device::ScreenShot(const std::string& file) const
{
    throw std::runtime_error("Not implemented!");
}

std::unique_ptr<frame::BufferInterface> Device::CreatePointBuffer(
    std::vector<float>&& vector)
{
    throw std::runtime_error("Not implemented!");
}

std::unique_ptr<frame::BufferInterface> Device::CreateIndexBuffer(
    std::vector<std::uint32_t>&& vector)
{
    throw std::runtime_error("Not implemented!");
}

std::unique_ptr<frame::StaticMeshInterface> Device::CreateStaticMesh(
    const StaticMeshParameter& static_mesh_parameter)
{
    throw std::runtime_error("Not implemented!");
}

std::unique_ptr<frame::TextureInterface> Device::CreateTexture(
    const TextureParameter& texture_parameter)
{
    throw std::runtime_error("Not implemented!");
}

} // End namespace frame::vulkan.
