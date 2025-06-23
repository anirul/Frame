#include "device.h"

#include <cmath>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <sstream>
#include <stdexcept>

#include "frame/camera.h"
#include "frame/file/image.h"
#include "frame/json/parse_uniform.h"
#include "frame/level.h"
#include "frame/opengl/cubemap.h"
#include "frame/opengl/frame_buffer.h"
#include "frame/opengl/render_buffer.h"
#include "frame/opengl/renderer.h"

namespace frame::opengl
{

Device::Device(void* gl_context, glm::uvec2 size)
    : gl_context_(gl_context), size_(size)
{
    // This should maintain the culling to none.
    // FIXME(anirul): Change this as to be working!
    glDisable(GL_CULL_FACE);
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_FRONT);
    // glFrontFace(GL_CCW);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    // Enable blending to 1 - source alpha.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    // Enable seamless cube map.
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

Device::~Device()
{
    Cleanup();
}

void Device::Startup(std::unique_ptr<frame::LevelInterface>&& level)
{
    // Copy level into the local area.
    level_ = std::move(level);
    // Setup camera.
    auto& camera = level_->GetDefaultCamera();
    camera.SetAspectRatio(
        static_cast<float>(size_.x) / static_cast<float>(size_.y));
    // Create a renderer.
    renderer_ = std::make_unique<Renderer>(
        *level_.get(), glm::uvec4(0, 0, size_.x, size_.y));
    // Add a callback to allow plugins to be called at pre-render step.
    renderer_->SetMeshRenderCallback([this](
                                         UniformCollectionInterface& uniform,
                                         StaticMeshInterface& static_mesh,
                                         MaterialInterface& material) {
        for (auto* plugin : GetPluginPtrs())
        {
            if (!plugin)
                continue;
            plugin->PreRender(uniform, *this, static_mesh, material);
        }
    });
}

void Device::AddPlugin(std::unique_ptr<PluginInterface>&& plugin_interface)
{
    std::string plugin_name = plugin_interface->GetName();
    for (int i = 0; i < plugin_interfaces_.size(); ++i)
    {
        if (plugin_interfaces_[i])
        {
            // If the plugin name is already in the list, then replace it.
            if (plugin_interfaces_[i]->GetName() == plugin_name)
            {
                plugin_interfaces_[i].reset();
                plugin_interfaces_[i] = std::move(plugin_interface);
                return;
            }
        }
    }
    for (int i = 0; i < plugin_interfaces_.size(); ++i)
    {
        // This is a free space add the plugin here.
        if (!plugin_interfaces_[i])
        {
            plugin_interfaces_[i] = std::move(plugin_interface);
            return;
        }
    }
    // No free space add the plugin at the end.
    plugin_interfaces_.push_back(std::move(plugin_interface));
}

std::vector<PluginInterface*> Device::GetPluginPtrs()
{
    std::vector<PluginInterface*> plugin_ptrs;
    for (auto& plugin_interface : plugin_interfaces_)
    {
        if (plugin_interface)
        {
            plugin_ptrs.push_back(plugin_interface.get());
        }
    }
    return plugin_ptrs;
}

std::vector<std::string> Device::GetPluginNames() const
{
    std::vector<std::string> names;
    for (const auto& plugin_interface : plugin_interfaces_)
    {
        if (plugin_interface)
        {
            names.push_back(plugin_interface->GetName());
        }
    }
    return names;
}

void Device::RemovePluginByName(const std::string& name)
{
    for (int i = 0; i < plugin_interfaces_.size(); ++i)
    {
        if (plugin_interfaces_[i])
        {
            if (plugin_interfaces_[i]->GetName() == name)
            {
                plugin_interfaces_[i].reset();
                return;
            }
        }
    }
}

void Device::Cleanup()
{
    renderer_ = nullptr;
}

void Device::Clear(
    const glm::vec4& color /* = glm::vec4(.2f, 0.f, .2f, 1.0f*/) const
{
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Device::DisplayCamera(
    const CameraInterface& camera, glm::uvec4 viewport, double time)
{
    if (!renderer_)
    {
        throw std::runtime_error("No Renderer.");
    }
    renderer_->SetDeltaTime(time);
    renderer_->PreRender();
    renderer_->SetViewport(viewport);
    renderer_->RenderShadows(camera);
    renderer_->RenderSkybox(camera);
    renderer_->RenderScene(camera);
    renderer_->PostProcess();
}

void Device::DisplayLeftRightCamera(
    const CameraInterface& camera_left,
    const CameraInterface& camera_right,
    glm::uvec4 viewport_left,
    glm::uvec4 viewport_right,
    double time)
{
    if (!renderer_)
    {
        throw std::runtime_error("No Renderer.");
    }
    renderer_->SetDeltaTime(time);
    renderer_->PreRender();
    renderer_->SetViewport(viewport_left);
    renderer_->RenderShadows(camera_left);
    renderer_->RenderSkybox(camera_left);
    if (invert_left_right_)
    {
        renderer_->SetViewport(viewport_right);
        renderer_->RenderScene(camera_left);
        renderer_->SetViewport(viewport_left);
        renderer_->RenderScene(camera_right);
        renderer_->PostProcess();
    }
    else
    {
        renderer_->SetViewport(viewport_left);
        renderer_->RenderScene(camera_left);
        renderer_->SetViewport(viewport_right);
        renderer_->RenderScene(camera_right);
        renderer_->PostProcess();
    }
}

void Device::Display(double dt /*= 0.0*/)
{
    if (!renderer_)
        throw std::runtime_error("No Renderer.");
    Clear();
    // Get the holder of the camera.
    auto camera_holder_id = level_->GetDefaultCameraId();
    auto enum_type = level_->GetEnumTypeFromId(camera_holder_id);
    auto& node = level_->GetSceneNodeFromId(camera_holder_id);
    auto matrix_node = node.GetLocalModel(dt);
    auto inverse_model = glm::inverse(matrix_node);
    CameraInterface& default_camera = level_->GetDefaultCamera();
    default_camera.SetFront(
        default_camera.GetFront() * glm::mat3(inverse_model));
    default_camera.SetPosition(glm::vec3(
        glm::vec4(default_camera.GetPosition(), 1.0) * inverse_model));
    // Compute left and right cameras.
    Camera left_camera{default_camera};
    left_camera.SetPosition(
        left_camera.GetPosition() -
        left_camera.GetRight() * interocular_distance_ * 0.5f);
    glm::vec3 left_camera_direction =
        default_camera.GetPosition() + focus_point_ - left_camera.GetPosition();
    left_camera.SetFront(glm::normalize(left_camera_direction));
    Camera right_camera{default_camera};
    right_camera.SetPosition(
        right_camera.GetPosition() +
        right_camera.GetRight() * interocular_distance_ * 0.5f);
    glm::vec3 right_camera_direction = default_camera.GetPosition() +
                                       focus_point_ -
                                       right_camera.GetPosition();
    right_camera.SetFront(glm::normalize(right_camera_direction));
    switch (stereo_enum_)
    {
    case StereoEnum::NONE:
        DisplayCamera(default_camera, glm::uvec4(0, 0, size_.x, size_.y), dt);
        break;
    case StereoEnum::HORIZONTAL_SPLIT:
        DisplayLeftRightCamera(
            left_camera,
            right_camera,
            glm::uvec4(0, 0, size_.x / 2, size_.y),
            glm::uvec4(size_.x / 2, 0, size_.x / 2, size_.y),
            dt);
        break;
    case StereoEnum::HORIZONTAL_SIDE_BY_SIDE:
        DisplayLeftRightCamera(
            left_camera,
            right_camera,
            glm::uvec4(0, 0, size_.x / 2, size_.y / 2),
            glm::uvec4(size_.x / 2, 0, size_.x / 2, size_.y / 2),
            dt);
        break;
    default:
        throw std::runtime_error(std::format(
            "Unknown StereoEnum type {}.", static_cast<int>(stereo_enum_)));
    }
    // Reset viewport.
    renderer_->SetViewport(glm::uvec4(0, 0, size_.x, size_.y));
    // Final display.
    renderer_->PresentFinal();
}

void Device::ScreenShot(const std::string& file) const
{
    auto maybe_texture_id = level_->GetDefaultOutputTextureId();
    if (!maybe_texture_id)
        throw std::runtime_error("no default texture.");
    auto texture_id = maybe_texture_id;
    auto& texture = level_->GetTextureFromId(texture_id);
    proto::PixelElementSize pixel_element_size{};
    pixel_element_size.set_value(
        texture.GetData().pixel_element_size().value());
    proto::PixelStructure pixel_structure{};
    pixel_structure.set_value(texture.GetData().pixel_structure().value());
    file::Image output_image(
        json::ParseSize(texture.GetData().size()),
        pixel_element_size,
        pixel_structure);
    auto vec = texture.GetTextureByte();
    output_image.SetData(vec.data());
    output_image.SaveImageToFile(file);
}

std::unique_ptr<frame::BufferInterface> Device::CreatePointBuffer(
    std::vector<float>&& vector)
{
    return opengl::CreatePointBuffer(std::move(vector));
}

std::unique_ptr<frame::BufferInterface> Device::CreateIndexBuffer(
    std::vector<std::uint32_t>&& vector)
{
    return opengl::CreateIndexBuffer(std::move(vector));
}

std::unique_ptr<frame::StaticMeshInterface> Device::CreateStaticMesh(
    const StaticMeshParameter& static_mesh_parameter)
{
    return std::make_unique<opengl::StaticMesh>(
        GetLevel(), static_mesh_parameter);
}

void Device::Resize(glm::uvec2 size)
{
    Cleanup();
    size_ = size;
    Startup(std::move(level_));
}

void Device::SetStereo(
    StereoEnum stereo_enum,
    float interocular_distance,
    glm::vec3 focus_point,
    bool invert_left_right)
{
    stereo_enum_ = stereo_enum;
    interocular_distance_ = interocular_distance;
    focus_point_ = focus_point;
    invert_left_right_ = invert_left_right;
}

glm::uvec2 Device::GetSize() const
{
    return size_;
}

} // End namespace frame::opengl.
