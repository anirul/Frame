#include "frame/vulkan/device.h"

namespace frame::vulkan {

Device::Device(void* vulkan_instance, glm::uvec2 size)
    : vulkan_instance_(vulkan_instance), size_(size) {
    logger_->debug("Implement the constructor for the class frame::vulkan::Device.");
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
