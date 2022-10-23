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

Device::Device(void* gl_context, const std::pair<std::uint32_t, std::uint32_t> size)
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
    camera->SetAspectRatio(static_cast<float>(size_.first) / static_cast<float>(size_.second));
    if (camera) camera->ComputeView();
    // Create a renderer.
    renderer_ = std::make_unique<Renderer>(level_.get(), size_);
}

void Device::Cleanup() { renderer_ = nullptr; }

void Device::Clear(const glm::vec4& color /* = glm::vec4(.2f, 0.f, .2f, 1.0f*/) const {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Device::Display(double dt /*= 0.0*/) {
    if (!renderer_) throw std::runtime_error("No Renderer.");
    Clear();
    renderer_->RenderAllMeshes(dt);
    // renderer_->RenderFromRootNode(dt);
    renderer_->Display(dt);
    if (level_) {
        Level* ptr = dynamic_cast<Level*>(level_.get());
        assert(ptr);
        auto last_program_id = renderer_->GetLastProgramId();
        if (last_program_id == NullId) logger_->warn("No last program id?");
        ptr->Update(this, ptr->GetProgramFromId(last_program_id), dt);
    }
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
    std::vector<float>&& vector, std::uint32_t point_buffer_size) {
    return opengl::CreateStaticMesh(GetLevel(), std::move(vector), point_buffer_size);
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

void Device::Resize(std::pair<std::uint32_t, std::uint32_t> size) {
    Cleanup();
    size_ = size;
    Startup(std::move(level_));
}

}  // End namespace frame::opengl.
