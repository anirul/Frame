#include "device.h"

#include <cmath>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <sstream>
#include <stdexcept>

#include "frame/file/image.h"
#include "frame/level.h"
#include "frame/opengl/frame_buffer.h"
#include "frame/opengl/render_buffer.h"
#include "frame/opengl/renderer.h"
#include "frame/opengl/texture_cube_map.h"

namespace frame::opengl {

Device::Device(void* gl_context, glm::uvec2 size)
    : gl_context_(gl_context), size_(size) {
    // This should maintain the culling to none.
    // FIXME(anirul): Change this as to be working!
    glDisable(GL_CULL_FACE);
    // glCullFace(GL_BACK);
    // glFrontFace(GL_CW);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    // Enable blending to 1 - source alpha.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    // Enable seamless cube map.
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

Device::~Device() { Cleanup(); }

void Device::Startup(std::unique_ptr<frame::LevelInterface>&& level) {
    // Copy level into the local area.
    level_ = std::move(level);
    // Setup camera.
    auto* camera = level_->GetDefaultCamera();
    if (camera) {
        camera->SetAspectRatio(static_cast<float>(size_.x) / static_cast<float>(size_.y));
    }
    // Create a renderer.
    Context context{ nullptr, this, level_.get() };
    renderer_ = std::make_unique<Renderer>(context, glm::uvec4(0, 0, size_.x, size_.y));
}

void Device::AddPlugin(std::unique_ptr<PluginInterface>&& plugin_interface) {
    std::string plugin_name = plugin_interface->GetName();
    for (int i = 0; i < plugin_interfaces_.size(); ++i) {
        if (plugin_interfaces_[i]) {
            // If the plugin name is already in the list, then replace it.
            if (plugin_interfaces_[i]->GetName() == plugin_name) {
                plugin_interfaces_[i].reset();
                plugin_interfaces_[i] = std::move(plugin_interface);
                return;
            }
        }
    }
    for (int i = 0; i < plugin_interfaces_.size(); ++i) {
        // This is a free space add the plugin here.
        if (!plugin_interfaces_[i]) {
            plugin_interfaces_[i] = std::move(plugin_interface);
            return;
        }
    }
    // No free space add the plugin at the end.
    plugin_interfaces_.push_back(std::move(plugin_interface));
}

std::vector<PluginInterface*> Device::GetPluginPtrs() {
    std::vector<PluginInterface*> plugin_ptrs;
    for (auto& plugin_interface : plugin_interfaces_) {
        if (plugin_interface) {
            plugin_ptrs.push_back(plugin_interface.get());
        }
    }
    return plugin_ptrs;
}

std::vector<std::string> Device::GetPluginNames() const {
    std::vector<std::string> names;
    for (const auto& plugin_interface : plugin_interfaces_) {
        if (plugin_interface) {
            names.push_back(plugin_interface->GetName());
        }
    }
    return names;
}

void Device::RemovePluginByName(const std::string& name) {
    for (int i = 0; i < plugin_interfaces_.size(); ++i) {
        if (plugin_interfaces_[i]) {
            if (plugin_interfaces_[i]->GetName() == name) {
                plugin_interfaces_[i].reset();
                return;
            }
        }
    }
}

void Device::Cleanup() { renderer_ = nullptr; }

void Device::Clear(const glm::vec4& color /* = glm::vec4(.2f, 0.f, .2f, 1.0f*/) const {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Device::DisplayCamera(const Camera& camera, glm::uvec4 viewport, double time) {
    renderer_->SetViewport(viewport);
    renderer_->RenderAllMeshes(camera.ComputeProjection(), camera.ComputeView(), time);
}

void Device::DisplayLeftRightCamera(const Camera& camera_left, const Camera& camera_right,
                                    glm::uvec4 viewport_left, glm::uvec4 viewport_right,
                                    double time) {
    if (invert_left_right_) {
        DisplayCamera(camera_right, viewport_left, time);
        DisplayCamera(camera_left, viewport_right, time);
    } else {
        DisplayCamera(camera_left, viewport_left, time);
        DisplayCamera(camera_right, viewport_right, time);
    }
}

void Device::Display(double dt /*= 0.0*/) {
    if (!renderer_) throw std::runtime_error("No Renderer.");
    Clear();
    // Compute left and right cameras.
    Camera left_camera = *level_->GetDefaultCamera();
    left_camera.SetPosition(left_camera.GetPosition() -
                            left_camera.GetRight() * interocular_distance_ * 0.5f);
    glm::vec3 left_camera_direction =
        level_->GetDefaultCamera()->GetPosition() + focus_point_ - left_camera.GetPosition();
    left_camera.SetFront(glm::normalize(left_camera_direction));
    Camera right_camera = *level_->GetDefaultCamera();
    right_camera.SetPosition(right_camera.GetPosition() +
                             right_camera.GetRight() * interocular_distance_ * 0.5f);
    glm::vec3 right_camera_direction =
        level_->GetDefaultCamera()->GetPosition() + focus_point_ - right_camera.GetPosition();
    right_camera.SetFront(glm::normalize(right_camera_direction));
    switch (stereo_enum_) {
        case StereoEnum::NONE:
            DisplayCamera(*level_->GetDefaultCamera(), glm::uvec4(0, 0, size_.x, size_.y),
                          dt);
            break;
        case StereoEnum::HORIZONTAL_SPLIT:
            DisplayLeftRightCamera(
                left_camera, right_camera, glm::uvec4(0, 0, size_.x / 2, size_.y),
                glm::uvec4(size_.x / 2, 0, size_.x / 2, size_.y), dt);
            break;
        case StereoEnum::HORIZONTAL_SIDE_BY_SIDE:
            DisplayLeftRightCamera(
                left_camera, right_camera, glm::uvec4(0, 0, size_.x / 2, size_.y / 2),
                glm::uvec4(size_.x / 2, 0, size_.x / 2, size_.y / 2), dt);
            break;
        default:
            throw std::runtime_error(
                fmt::format("Unknown StereoEnum type {}.", static_cast<int>(stereo_enum_)));
    }
    // Reset viewport.
    renderer_->SetViewport(glm::uvec4(0, 0, size_.x, size_.y));
    // Final display.
    // CHECKME(anirul): Is this still needed?
    renderer_->Display(dt);
}

void Device::ScreenShot(const std::string& file) const {
    auto maybe_texture_id = level_->GetDefaultOutputTextureId();
    if (!maybe_texture_id) throw std::runtime_error("no default texture.");
    auto texture_id = maybe_texture_id;
    auto* texture   = level_->GetTextureFromId(texture_id);
    if (!texture) throw std::runtime_error("could not open texture.");
    proto::PixelElementSize pixel_element_size{};
    pixel_element_size.set_value(texture->GetPixelElementSize());
    proto::PixelStructure pixel_structure{};
    pixel_structure.set_value(texture->GetPixelStructure());
    file::Image output_image(texture->GetSize(), pixel_element_size, pixel_structure);
    auto vec = texture->GetTextureByte();
    output_image.SetData(vec.data());
    output_image.SaveImageToFile(file);
}

std::unique_ptr<frame::BufferInterface> Device::CreatePointBuffer(std::vector<float>&& vector) {
    return opengl::CreatePointBuffer(std::move(vector));
}

std::unique_ptr<frame::BufferInterface> Device::CreateIndexBuffer(
    std::vector<std::uint32_t>&& vector) {
    return opengl::CreateIndexBuffer(std::move(vector));
}

std::unique_ptr<frame::StaticMeshInterface> Device::CreateStaticMesh(
    const StaticMeshParameter& static_mesh_parameter) {
    return std::make_unique<opengl::StaticMesh>(GetLevel(), static_mesh_parameter);
}

std::unique_ptr<frame::TextureInterface> Device::CreateTexture(
    const TextureParameter& texture_parameter) {
    switch (texture_parameter.map_type) {
        case TextureTypeEnum::TEXTURE_2D:
            return std::make_unique<Texture>(texture_parameter);
        case TextureTypeEnum::CUBMAP:
            return std::make_unique<TextureCubeMap>(texture_parameter);
        case TextureTypeEnum::TEXTURE_3D:
            throw std::runtime_error("No 3D texture implemented yet!");
        default:
            throw std::runtime_error("Unknown texture type?");
    }
}

void Device::Resize(glm::uvec2 size) {
    Cleanup();
    size_ = size;
    Startup(std::move(level_));
}

void Device::SetStereo(StereoEnum stereo_enum, float interocular_distance, glm::vec3 focus_point,
                       bool invert_left_right) {
    stereo_enum_          = stereo_enum;
    interocular_distance_ = interocular_distance;
    focus_point_          = focus_point;
    invert_left_right_    = invert_left_right;
}

}  // End namespace frame::opengl.
