#include "frame/vulkan/device.h"

namespace frame::vulkan {

Device::Device(void* vk_instance, glm::uvec2 size, vk::SurfaceKHR& surface,
               vk::DispatchLoaderDynamic& dispatch)
    : vk_instance_(static_cast<VkInstance>(vk_instance)),
      size_(size),
      vk_surface_(surface),
      vk_dispatch_loader_(dispatch) {
    logger_->info("Creating Vulkan Device");
    std::vector<vk::PhysicalDevice> physical_devices = vk_instance_.enumeratePhysicalDevices();
    if (physical_devices.empty()) {
        throw std::runtime_error("No Vulkan Physical Device found");
    }
    // Check and select physical device properties.
    int last_best_score = 0;
    for (const auto& physical_device : physical_devices) {
        int score = 0;
        logger_->info("Physical Device: {}", physical_device.getProperties().deviceName);
        if (physical_device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            logger_->info("\tis a GPU");
            score += 10000;
        }
        score += physical_device.getProperties().limits.maxImageDimension2D;
        if (physical_device.getFeatures().geometryShader) {
            if (score > last_best_score) {
                last_best_score     = score;
                vk_physical_device_ = physical_device;
            }
        }
    }
    if (!vk_physical_device_) {
        throw std::runtime_error("No Vulkan Physical Device found");
    }
    // Select a queue family.
    std::vector<vk::QueueFamilyProperties> queue_families =
        vk_physical_device_.getQueueFamilyProperties(vk_dispatch_loader_);
    int i              = 0;
    int selected_index = -1;
    for (auto& queue_family : queue_families) {
        if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics) {
            selected_index = i;
        }
        i++;
    }
    if (selected_index == -1) {
        throw std::runtime_error("No Vulkan Queue Family found");
    }
    // Get the device.
    vk::DeviceQueueCreateInfo device_queue_create_info({}, selected_index, 1,
                                                       &queue_family_priority_);
    vk::DeviceCreateInfo device_create_info({}, 1, &device_queue_create_info);
    vk_unique_device_ =
        vk_physical_device_.createDeviceUnique(device_create_info, nullptr, vk_dispatch_loader_);
}

Device::~Device() {}

void Device::SetStereo(StereoEnum stereo_enum, float interocular_distance, glm::vec3 focus_point,
                       bool invert_left_right) {
    throw std::runtime_error("Not implemented!");
}

void Device::Clear(const glm::vec4& color /*= glm::vec4(.2f, 0.f, .2f, 1.0f)*/) const {
    throw std::runtime_error("Not implemented!");
}

void Device::Startup(std::unique_ptr<LevelInterface>&& level) {
    throw std::runtime_error("Not implemented!");
}

void Device::AddPlugin(std::unique_ptr<PluginInterface>&& plugin_interface) {
    throw std::runtime_error("Not implemented!");
}

std::vector<PluginInterface*> Device::GetPluginPtrs() {
    throw std::runtime_error("Not implemented!");
}

std::vector<std::string> Device::GetPluginNames() const {
    throw std::runtime_error("Not implemented!");
}

void Device::RemovePluginByName(const std::string& name) {
    throw std::runtime_error("Not implemented!");
}

void Device::Cleanup() { throw std::runtime_error("Not implemented!"); }

void Device::Resize(glm::uvec2 size) { throw std::runtime_error("Not implemented!"); }

void Device::Display(double dt /*= 0.0*/) { throw std::runtime_error("Not implemented!"); }

void Device::ScreenShot(const std::string& file) const {
    throw std::runtime_error("Not implemented!");
}

std::unique_ptr<frame::BufferInterface> Device::CreatePointBuffer(std::vector<float>&& vector) {
    throw std::runtime_error("Not implemented!");
}

std::unique_ptr<frame::BufferInterface> Device::CreateIndexBuffer(
    std::vector<std::uint32_t>&& vector) {
    throw std::runtime_error("Not implemented!");
}

std::unique_ptr<frame::StaticMeshInterface> Device::CreateStaticMesh(
    const StaticMeshParameter& static_mesh_parameter) {
    throw std::runtime_error("Not implemented!");
}

std::unique_ptr<frame::TextureInterface> Device::CreateTexture(
    const TextureParameter& texture_parameter) {
    throw std::runtime_error("Not implemented!");
}

}  // End namespace frame::vulkan.
