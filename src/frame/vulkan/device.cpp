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
        logger_->info("\t{}", extension_name);
    }
    // List all available layers.
    std::vector<vk::LayerProperties> availableLayers =
        vk::enumerateInstanceLayerProperties();
    logger_->info("Available Layers:");
    for (const auto& layer : availableLayers)
    {
        std::string layer_name = layer.layerName;
        logger_->info("\t{}", layer_name);
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
    // Get the physical devices.
    vk::raii::PhysicalDevices physical_devices(vk_instance_.value());
    std::uint32_t selected_physical_device_index = -1;
    vk::PhysicalDeviceFeatures device_features;
    for (std::uint32_t i = 0; i < physical_devices.size(); ++i)
    {
        auto properties = physical_devices[i].getProperties();
        auto features = physical_devices[i].getFeatures();
        std::string device_name = properties.deviceName;
        logger_->info("\t{}", device_name);
        // TODO(anirul): make the checks.
        selected_physical_device_index = i;
        device_features = features;
    }
    if (selected_physical_device_index == -1)
    {
        throw std::runtime_error("No physical device found!");
    }
    vk_physical_device_.emplace(
        std::move(
        physical_devices[selected_physical_device_index]));
    // Get the queue family properties.
    std::vector<vk::QueueFamilyProperties> queue_family_properties =
        vk_physical_device_.value().getQueueFamilyProperties();
    std::uint32_t selected_queue_family_index = -1;
    for (std::uint32_t i = 0; i < queue_family_properties.size(); ++i)
    {
        auto queue_flags = queue_family_properties[i].queueFlags;
        if (queue_flags & vk::QueueFlagBits::eGraphics)
        {
            selected_queue_family_index = i;
            break;
        }
    }
    if (selected_queue_family_index == -1)
    {
        throw std::runtime_error("No queue family found!");
    }
    float queque_priority = 1.0f;
    vk::DeviceQueueCreateInfo queue_create_info(
        {},
        selected_queue_family_index,
        1,
        &queque_priority);
    // Create the device.
    vk::DeviceCreateInfo device_create_info({}, queue_create_info, {}, {}, &device_features);
    vk_device_.emplace(vk_physical_device_.value(), device_create_info);
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
