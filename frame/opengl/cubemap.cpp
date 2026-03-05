#include "frame/opengl/cubemap.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <glad/glad.h>
#include <numbers>
#include <stdexcept>

#include "frame/file/file_system.h"
#include "frame/file/image.h"
#include "frame/json/parse_uniform.h"
#include "frame/json/serialize_uniform.h"
#include "frame/opengl/frame_buffer.h"
#include "frame/opengl/pixel.h"
#include "frame/opengl/render_buffer.h"

namespace frame::opengl
{

namespace
{
std::uint32_t PowerFloor(std::uint32_t x)
{
    std::uint32_t power = 1;
    while (x >>= 1)
    {
        power <<= 1;
    }
    return power;
}

std::uint8_t BytesPerComponent(frame::proto::PixelElementSize::Enum element_size)
{
    switch (element_size)
    {
    case frame::proto::PixelElementSize::BYTE:
        return 1;
    case frame::proto::PixelElementSize::SHORT:
        return 2;
    case frame::proto::PixelElementSize::HALF:
        // OpenGL upload path uses GL_FLOAT for HALF textures in this engine.
        return 4;
    case frame::proto::PixelElementSize::FLOAT:
        return 4;
    default:
        throw std::runtime_error("Unsupported pixel element size for cubemap.");
    }
}

std::uint8_t ComponentCount(frame::proto::PixelStructure::Enum structure)
{
    switch (structure)
    {
    case frame::proto::PixelStructure::GREY:
    case frame::proto::PixelStructure::DEPTH:
        return 1;
    case frame::proto::PixelStructure::GREY_ALPHA:
        return 2;
    case frame::proto::PixelStructure::RGB:
    case frame::proto::PixelStructure::BGR:
        return 3;
    case frame::proto::PixelStructure::RGB_ALPHA:
    case frame::proto::PixelStructure::BGR_ALPHA:
        return 4;
    default:
        throw std::runtime_error("Unsupported pixel structure for cubemap.");
    }
}

float ReadComponent(
    const std::uint8_t* data,
    std::size_t base_offset,
    std::size_t channel,
    frame::proto::PixelElementSize::Enum element_size)
{
    switch (element_size)
    {
    case frame::proto::PixelElementSize::BYTE:
        return static_cast<float>(data[base_offset + channel]) / 255.0f;
    case frame::proto::PixelElementSize::SHORT: {
        std::uint16_t value = 0;
        std::memcpy(
            &value,
            data + base_offset + channel * sizeof(std::uint16_t),
            sizeof(value));
        return static_cast<float>(value) / 65535.0f;
    }
    case frame::proto::PixelElementSize::HALF:
        [[fallthrough]];
    case frame::proto::PixelElementSize::FLOAT: {
        float value = 0.0f;
        std::memcpy(
            &value,
            data + base_offset + channel * sizeof(float),
            sizeof(value));
        return value;
    }
    default:
        throw std::runtime_error("Unsupported pixel element size for cubemap.");
    }
}

glm::vec4 ReadEquirectangularPixel(
    const std::uint8_t* data,
    glm::uvec2 source_size,
    std::uint32_t x,
    std::uint32_t y,
    frame::proto::PixelElementSize::Enum element_size,
    frame::proto::PixelStructure::Enum structure)
{
    const std::uint8_t component_count = ComponentCount(structure);
    const std::uint8_t bytes_per_component = BytesPerComponent(element_size);
    const std::size_t bytes_per_pixel =
        static_cast<std::size_t>(component_count) * bytes_per_component;
    const std::size_t index =
        static_cast<std::size_t>(y) * source_size.x + x;
    const std::size_t base_offset = index * bytes_per_pixel;

    auto component = [&](std::size_t channel) {
        if (channel >= component_count)
        {
            return 1.0f;
        }
        return ReadComponent(data, base_offset, channel, element_size);
    };

    switch (structure)
    {
    case frame::proto::PixelStructure::GREY:
    case frame::proto::PixelStructure::DEPTH: {
        const float value = component(0);
        return glm::vec4(value, value, value, 1.0f);
    }
    case frame::proto::PixelStructure::GREY_ALPHA: {
        const float value = component(0);
        return glm::vec4(value, value, value, component(1));
    }
    case frame::proto::PixelStructure::RGB:
        return glm::vec4(component(0), component(1), component(2), 1.0f);
    case frame::proto::PixelStructure::BGR:
        return glm::vec4(component(2), component(1), component(0), 1.0f);
    case frame::proto::PixelStructure::RGB_ALPHA:
        return glm::vec4(component(0), component(1), component(2), component(3));
    case frame::proto::PixelStructure::BGR_ALPHA:
        return glm::vec4(component(2), component(1), component(0), component(3));
    default:
        throw std::runtime_error("Unsupported pixel structure for cubemap.");
    }
}

void WriteComponent(
    std::vector<std::uint8_t>& destination,
    std::size_t base_offset,
    std::size_t channel,
    float value,
    frame::proto::PixelElementSize::Enum element_size)
{
    switch (element_size)
    {
    case frame::proto::PixelElementSize::BYTE: {
        const float clamped = std::clamp(value, 0.0f, 1.0f);
        destination[base_offset + channel] =
            static_cast<std::uint8_t>(clamped * 255.0f);
        break;
    }
    case frame::proto::PixelElementSize::SHORT: {
        const float clamped = std::clamp(value, 0.0f, 1.0f);
        const std::uint16_t converted =
            static_cast<std::uint16_t>(clamped * 65535.0f);
        std::memcpy(
            destination.data() + base_offset + channel * sizeof(std::uint16_t),
            &converted,
            sizeof(converted));
        break;
    }
    case frame::proto::PixelElementSize::HALF:
        [[fallthrough]];
    case frame::proto::PixelElementSize::FLOAT: {
        std::memcpy(
            destination.data() + base_offset + channel * sizeof(float),
            &value,
            sizeof(value));
        break;
    }
    default:
        throw std::runtime_error("Unsupported pixel element size for cubemap.");
    }
}

void WriteCubemapPixel(
    std::vector<std::uint8_t>& destination,
    std::size_t base_offset,
    const glm::vec4& rgba,
    frame::proto::PixelElementSize::Enum element_size,
    frame::proto::PixelStructure::Enum structure)
{
    switch (structure)
    {
    case frame::proto::PixelStructure::GREY:
    case frame::proto::PixelStructure::DEPTH:
        WriteComponent(destination, base_offset, 0, rgba.r, element_size);
        break;
    case frame::proto::PixelStructure::GREY_ALPHA:
        WriteComponent(destination, base_offset, 0, rgba.r, element_size);
        WriteComponent(destination, base_offset, 1, rgba.a, element_size);
        break;
    case frame::proto::PixelStructure::RGB:
        WriteComponent(destination, base_offset, 0, rgba.r, element_size);
        WriteComponent(destination, base_offset, 1, rgba.g, element_size);
        WriteComponent(destination, base_offset, 2, rgba.b, element_size);
        break;
    case frame::proto::PixelStructure::BGR:
        WriteComponent(destination, base_offset, 0, rgba.b, element_size);
        WriteComponent(destination, base_offset, 1, rgba.g, element_size);
        WriteComponent(destination, base_offset, 2, rgba.r, element_size);
        break;
    case frame::proto::PixelStructure::RGB_ALPHA:
        WriteComponent(destination, base_offset, 0, rgba.r, element_size);
        WriteComponent(destination, base_offset, 1, rgba.g, element_size);
        WriteComponent(destination, base_offset, 2, rgba.b, element_size);
        WriteComponent(destination, base_offset, 3, rgba.a, element_size);
        break;
    case frame::proto::PixelStructure::BGR_ALPHA:
        WriteComponent(destination, base_offset, 0, rgba.b, element_size);
        WriteComponent(destination, base_offset, 1, rgba.g, element_size);
        WriteComponent(destination, base_offset, 2, rgba.r, element_size);
        WriteComponent(destination, base_offset, 3, rgba.a, element_size);
        break;
    default:
        throw std::runtime_error("Unsupported pixel structure for cubemap.");
    }
}

glm::vec3 FaceDirection(int face_index, glm::vec2 uv)
{
    const float a = 2.0f * uv.x - 1.0f;
    const float b = 2.0f * uv.y - 1.0f;
    switch (face_index)
    {
    case 0: return glm::normalize(glm::vec3(1.0f, -b, -a));  // +X
    case 1: return glm::normalize(glm::vec3(-1.0f, -b, a));  // -X
    case 2: return glm::normalize(glm::vec3(a, 1.0f, b));    // +Y
    case 3: return glm::normalize(glm::vec3(a, -1.0f, -b));  // -Y
    case 4: return glm::normalize(glm::vec3(a, -b, 1.0f));   // +Z
    case 5: return glm::normalize(glm::vec3(-a, -b, -1.0f)); // -Z
    default:
        throw std::runtime_error("Invalid cubemap face index.");
    }
}

} // End namespace.

Cubemap::~Cubemap()
{
    glDeleteTextures(1, &texture_id_);
}

Cubemap::Cubemap(
    std::filesystem::path file_name,
    proto::PixelElementSize pixel_element_size,
    proto::PixelStructure pixel_structure)
{
    data_.set_cubemap(true);
    CreateCubemapFromFile(file_name, pixel_element_size, pixel_structure);
}

Cubemap::Cubemap(
    std::array<std::filesystem::path, 6> file_names,
    proto::PixelElementSize pixel_element_size,
    proto::PixelStructure pixel_structure)
{
    data_.set_cubemap(true);
    CreateCubemapFromFiles(file_names, pixel_element_size, pixel_structure);
}

Cubemap::Cubemap(
    const void* ptr,
    glm::uvec2 size,
    proto::PixelElementSize pixel_element_size,
    proto::PixelStructure pixel_structure)
{
    data_.set_cubemap(true);
    CreateCubemapFromPointer(ptr, size, pixel_element_size, pixel_structure);
}

Cubemap::Cubemap(
    std::array<const void*, 6> ptrs,
    glm::uvec2 size,
    proto::PixelElementSize pixel_element_size,
    proto::PixelStructure pixel_structure)
{
    data_.set_cubemap(true);
    CreateCubemapFromPointers(ptrs, size, pixel_element_size, pixel_structure);
}

Cubemap::Cubemap(const proto::Texture& proto_texture, glm::uvec2 display_size)
{
    data_ = proto_texture;
    data_.set_cubemap(true);
    SetDisplaySize(display_size);
    switch (proto_texture.texture_oneof_case())
    {
    case proto::Texture::kPlugin:
        [[fallthrough]];
    case proto::Texture::kPixels:
        throw std::runtime_error("Not supported yet.");
    case proto::Texture::kFileName:
        if (proto_texture.file_name().empty())
        {
            CreateCubemapFromPointers(
                {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
                GetSize(),
                proto_texture.pixel_element_size(),
                proto_texture.pixel_structure());
        }
        else
        {
            CreateCubemapFromFile(
                proto_texture.file_name(),
                proto_texture.pixel_element_size(),
                proto_texture.pixel_structure());
        }
        break;
    case proto::Texture::kFileNames:
        CreateCubemapFromFiles(
            std::array<std::filesystem::path, 6>{
                proto_texture.file_names().positive_x(),
                proto_texture.file_names().negative_x(),
                proto_texture.file_names().positive_y(),
                proto_texture.file_names().negative_y(),
                proto_texture.file_names().positive_z(),
                proto_texture.file_names().negative_z()},
            proto_texture.pixel_element_size(),
            proto_texture.pixel_structure());
        break;
    case 0:
        CreateCubemapFromPointers(
            std::array<const void*, 6>{
                nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
            GetSize(),
            proto_texture.pixel_element_size(),
            proto_texture.pixel_structure());
        break;
    default:
        throw std::runtime_error(std::format("Unknown type?"));
    }
}

void Cubemap::Bind(unsigned int slot /*= 0*/) const
{
    assert(slot < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
    if (locked_bind_)
    {
        return;
    }
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id_);
}

void Cubemap::UnBind() const
{
    if (locked_bind_)
    {
        return;
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

int Cubemap::ConvertToGLType(
    const proto::TextureFilter::Enum texture_filter) const
{
    switch (texture_filter)
    {
    case frame::proto::TextureFilter::NEAREST:
        return GL_NEAREST;
    case frame::proto::TextureFilter::LINEAR:
        return GL_LINEAR;
    case frame::proto::TextureFilter::NEAREST_MIPMAP_NEAREST:
        return GL_NEAREST_MIPMAP_NEAREST;
    case frame::proto::TextureFilter::LINEAR_MIPMAP_NEAREST:
        return GL_LINEAR_MIPMAP_NEAREST;
    case frame::proto::TextureFilter::NEAREST_MIPMAP_LINEAR:
        return GL_NEAREST_MIPMAP_LINEAR;
    case frame::proto::TextureFilter::LINEAR_MIPMAP_LINEAR:
        return GL_LINEAR_MIPMAP_LINEAR;
    case frame::proto::TextureFilter::CLAMP_TO_EDGE:
        return GL_CLAMP_TO_EDGE;
    case frame::proto::TextureFilter::MIRRORED_REPEAT:
        return GL_MIRRORED_REPEAT;
    case frame::proto::TextureFilter::REPEAT:
        return GL_REPEAT;
    default:
        throw std::runtime_error(
            "Invalid texture filter : " +
            std::to_string(static_cast<int>(texture_filter)));
    }
}

frame::proto::TextureFilter::Enum Cubemap::ConvertFromGLType(
    int gl_filter) const
{
    switch (gl_filter)
    {
    case GL_NEAREST:
        return frame::proto::TextureFilter::NEAREST;
    case GL_LINEAR:
        return frame::proto::TextureFilter::LINEAR;
    case GL_NEAREST_MIPMAP_NEAREST:
        return frame::proto::TextureFilter::NEAREST_MIPMAP_NEAREST;
    case GL_LINEAR_MIPMAP_NEAREST:
        return frame::proto::TextureFilter::LINEAR_MIPMAP_NEAREST;
    case GL_NEAREST_MIPMAP_LINEAR:
        return frame::proto::TextureFilter::NEAREST_MIPMAP_LINEAR;
    case GL_LINEAR_MIPMAP_LINEAR:
        return frame::proto::TextureFilter::LINEAR_MIPMAP_LINEAR;
    case GL_CLAMP_TO_EDGE:
        return frame::proto::TextureFilter::CLAMP_TO_EDGE;
    case GL_MIRRORED_REPEAT:
        return frame::proto::TextureFilter::MIRRORED_REPEAT;
    case GL_REPEAT:
        return frame::proto::TextureFilter::REPEAT;
    }
    throw std::runtime_error(
        "invalid texture filter : " + std::to_string(gl_filter));
}

void Cubemap::CreateFrameAndRenderBuffer()
{
    frame_ = std::make_unique<FrameBuffer>();
    render_ = std::make_unique<RenderBuffer>();
    render_->CreateStorage(json::ParseSize(data_.size()));
    frame_->AttachRender(*render_);
    frame_->AttachTexture(GetId());
    frame_->DrawBuffers(1);
}

void Cubemap::CreateCubemapFromFile(
    std::filesystem::path file_name,
    proto::PixelElementSize pixel_element_size,
    proto::PixelStructure pixel_structure)
{
    data_.set_cubemap(true);
    data_.mutable_pixel_element_size()->CopyFrom(pixel_element_size);
    data_.mutable_pixel_structure()->CopyFrom(pixel_structure);
    data_.set_file_name(frame::file::PurifyFilePath(file_name));
    frame::file::Image image(
        file_name,
        data_.pixel_element_size(),
        data_.pixel_structure());
    CreateCubemapFromPointer(
        image.Data(),
        image.GetSize(),
        data_.pixel_element_size(),
        data_.pixel_structure());
}

void Cubemap::CreateCubemapFromFiles(
    std::array<std::filesystem::path, 6> file_names,
    proto::PixelElementSize pixel_element_size,
    proto::PixelStructure pixel_structure)
{
    data_.set_cubemap(true);
    data_.mutable_pixel_element_size()->CopyFrom(pixel_element_size);
    data_.mutable_pixel_structure()->CopyFrom(pixel_structure);
    std::vector<std::unique_ptr<frame::file::Image>> images;
    std::vector<std::filesystem::path> paths;
    for (const std::filesystem::path& file_name : file_names)
    {
        paths.push_back(file_name);
        auto image_ptr = std::make_unique<frame::file::Image>(
            file_name,
            pixel_element_size,
            pixel_structure);
        images.push_back(std::move(image_ptr));
    }
    proto::CubemapFiles cubemap_files;
    cubemap_files.set_positive_x(frame::file::PurifyFilePath(paths[0]));
    cubemap_files.set_negative_x(frame::file::PurifyFilePath(paths[1]));
    cubemap_files.set_positive_y(frame::file::PurifyFilePath(paths[2]));
    cubemap_files.set_negative_y(frame::file::PurifyFilePath(paths[3]));
    cubemap_files.set_positive_z(frame::file::PurifyFilePath(paths[4]));
    cubemap_files.set_negative_z(frame::file::PurifyFilePath(paths[5]));
    data_.mutable_file_names()->CopyFrom(cubemap_files);
    CreateCubemapFromPointers(
        {images[0]->Data(),
         images[1]->Data(),
         images[2]->Data(),
         images[3]->Data(),
         images[4]->Data(),
         images[5]->Data()},
        images[0]->GetSize(),
        pixel_element_size,
        pixel_structure);
}

void Cubemap::CreateCubemapFromPointer(
    const void* ptr,
    glm::uvec2 size,
    proto::PixelElementSize pixel_element_size,
    proto::PixelStructure pixel_structure)
{
    data_.set_cubemap(true);
    data_.mutable_pixel_element_size()->CopyFrom(pixel_element_size);
    data_.mutable_pixel_structure()->CopyFrom(pixel_structure);
    if (!ptr || size.x == 0 || size.y == 0)
    {
        throw std::runtime_error(
            "Invalid equirectangular texture input for cubemap creation.");
    }

    // Cap cubemap face resolution to avoid excessive GPU memory usage on large
    // HDR inputs.
    constexpr std::uint32_t kMaxCubemapFaceResolution = 1024u;
    std::uint32_t cube_single_res = std::min(
        PowerFloor(size.y), kMaxCubemapFaceResolution);
    if (cube_single_res == 0)
    {
        throw std::runtime_error(
            "Unable to compute a valid cubemap resolution from source texture.");
    }
    glm::uvec2 cube_size = {cube_single_res, cube_single_res};

    const auto element_enum =
        static_cast<frame::proto::PixelElementSize::Enum>(
            pixel_element_size.value());
    const auto structure_enum =
        static_cast<frame::proto::PixelStructure::Enum>(
            pixel_structure.value());
    const std::uint8_t component_count = ComponentCount(structure_enum);
    const std::uint8_t bytes_per_component = BytesPerComponent(element_enum);
    const std::size_t bytes_per_pixel =
        static_cast<std::size_t>(component_count) * bytes_per_component;
    const std::size_t face_pixel_count =
        static_cast<std::size_t>(cube_size.x) * cube_size.y;

    std::vector<std::uint8_t> cube_data(
        face_pixel_count * 6 * bytes_per_pixel,
        0);
    const auto* source_data = static_cast<const std::uint8_t*>(ptr);

    for (int face_index = 0; face_index < 6; ++face_index)
    {
        for (std::uint32_t y = 0; y < cube_size.y; ++y)
        {
            for (std::uint32_t x = 0; x < cube_size.x; ++x)
            {
                const glm::vec2 uv(
                    (static_cast<float>(x) + 0.5f) /
                        static_cast<float>(cube_size.x),
                    (static_cast<float>(y) + 0.5f) /
                        static_cast<float>(cube_size.y));
                const glm::vec3 direction = FaceDirection(face_index, uv);
                const float theta = std::atan2(direction.z, direction.x);
                const float phi =
                    std::asin(std::clamp(direction.y, -1.0f, 1.0f));
                const glm::vec2 equi_uv(
                    0.5f + theta / (2.0f * std::numbers::pi_v<float>),
                    0.5f - phi / std::numbers::pi_v<float>);

                const float clamped_u = std::clamp(equi_uv.x, 0.0f, 0.999999f);
                const float clamped_v = std::clamp(equi_uv.y, 0.0f, 0.999999f);
                const std::uint32_t source_x = static_cast<std::uint32_t>(
                    clamped_u * static_cast<float>(size.x));
                const std::uint32_t source_y = static_cast<std::uint32_t>(
                    clamped_v * static_cast<float>(size.y));
                const glm::vec4 source_pixel = ReadEquirectangularPixel(
                    source_data,
                    size,
                    source_x,
                    source_y,
                    element_enum,
                    structure_enum);

                const std::size_t destination_pixel_index =
                    static_cast<std::size_t>(face_index) * face_pixel_count +
                    static_cast<std::size_t>(y) * cube_size.x + x;
                const std::size_t destination_base =
                    destination_pixel_index * bytes_per_pixel;
                WriteCubemapPixel(
                    cube_data,
                    destination_base,
                    source_pixel,
                    element_enum,
                    structure_enum);
            }
        }
    }

    std::array<const void*, 6> face_ptrs = {};
    for (std::size_t face_index = 0; face_index < face_ptrs.size(); ++face_index)
    {
        face_ptrs[face_index] =
            cube_data.data() + face_index * face_pixel_count * bytes_per_pixel;
    }
    CreateCubemapFromPointers(
        face_ptrs, cube_size, pixel_element_size, pixel_structure);

    if (!data_.has_size())
        data_.mutable_size()->CopyFrom(json::SerializeSize(inner_size_));
}

void Cubemap::CreateCubemapFromPointers(
    std::array<const void*, 6> cube_map,
    glm::uvec2 size,
    proto::PixelElementSize pixel_element_size,
    proto::PixelStructure pixel_structure)
{
    data_.set_cubemap(true);
    data_.mutable_pixel_element_size()->CopyFrom(pixel_element_size);
    data_.mutable_pixel_structure()->CopyFrom(pixel_structure);
    inner_size_ = size;
    if (!data_.has_size())
        data_.mutable_size()->CopyFrom(json::SerializeSize(inner_size_));
    glGenTextures(1, &texture_id_);
    ScopedBind scoped_bind(*this);
    glTexParameteri(
        GL_TEXTURE_CUBE_MAP,
        GL_TEXTURE_MIN_FILTER,
        ConvertToGLType(proto::TextureFilter::LINEAR));
    glTexParameteri(
        GL_TEXTURE_CUBE_MAP,
        GL_TEXTURE_MAG_FILTER,
        ConvertToGLType(proto::TextureFilter::LINEAR));
    glTexParameteri(
        GL_TEXTURE_CUBE_MAP,
        GL_TEXTURE_WRAP_S,
        ConvertToGLType(proto::TextureFilter::CLAMP_TO_EDGE));
    glTexParameteri(
        GL_TEXTURE_CUBE_MAP,
        GL_TEXTURE_WRAP_T,
        ConvertToGLType(proto::TextureFilter::CLAMP_TO_EDGE));
    glTexParameteri(
        GL_TEXTURE_CUBE_MAP,
        GL_TEXTURE_WRAP_R,
        ConvertToGLType(proto::TextureFilter::CLAMP_TO_EDGE));
    for (unsigned int i : {0, 1, 2, 3, 4, 5})
    {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0,
            opengl::ConvertToGLType(
                data_.pixel_element_size(), data_.pixel_structure()),
            static_cast<GLsizei>(inner_size_.x),
            static_cast<GLsizei>(inner_size_.y),
            0,
            opengl::ConvertToGLType(data_.pixel_structure()),
            opengl::ConvertToGLType(data_.pixel_element_size()),
            cube_map[i]);
    }
}

void Cubemap::Clear(glm::vec4 color)
{
    // First time this is called this will create a frame and a render.
    Bind();
    if (!frame_)
    {
        CreateFrameAndRenderBuffer();
    }
    ScopedBind scoped_frame(*frame_);
    glViewport(0, 0, inner_size_.x, inner_size_.y);
    GLfloat clear_color[4] = {color.r, color.g, color.b, color.a};
    glClearBufferfv(GL_COLOR, 0, clear_color);
    UnBind();
}

std::vector<std::uint8_t> Cubemap::GetTextureByte() const
{
    auto format = opengl::ConvertToGLType(data_.pixel_structure());
    auto type = opengl::ConvertToGLType(data_.pixel_element_size());
    if (type != GL_UNSIGNED_BYTE)
    {
        throw std::runtime_error(
            "Invalid format should be unsigned byte is : " +
            proto::PixelElementSize_Enum_Name(
                data_.pixel_element_size().value()));
    }
    auto pixel_structure = data_.pixel_structure().value();
    // 6 because of cubemap!
    std::size_t image_size = static_cast<std::size_t>(inner_size_.x) *
                             static_cast<std::size_t>(inner_size_.y) *
                             static_cast<std::size_t>(pixel_structure) * 6;
    std::vector<std::uint8_t> result = {};
    result.resize(image_size);
    glGetTextureImage(
        texture_id_,
        0,
        format,
        type,
        static_cast<GLsizei>(image_size * sizeof(std::uint8_t)),
        result.data());
    return result;
}

std::vector<std::uint16_t> Cubemap::GetTextureWord() const
{
    auto format = opengl::ConvertToGLType(data_.pixel_structure());
    auto type = opengl::ConvertToGLType(data_.pixel_element_size());
    if (type != GL_UNSIGNED_SHORT)
    {
        throw std::runtime_error(
            "Invalid format should be unsigned short is : " +
            proto::PixelElementSize_Enum_Name(
                data_.pixel_element_size().value()));
    }
    auto pixel_structure = data_.pixel_structure().value();
    // 6 because of cubemap!
    std::size_t image_size = static_cast<std::size_t>(inner_size_.x) *
                             static_cast<std::size_t>(inner_size_.y) *
                             static_cast<std::size_t>(pixel_structure) * 6;
    std::vector<std::uint16_t> result = {};
    result.resize(image_size);
    glGetTextureImage(
        texture_id_,
        0,
        format,
        type,
        static_cast<GLsizei>(image_size * sizeof(std::uint16_t)),
        result.data());
    return result;
}

std::vector<std::uint32_t> Cubemap::GetTextureDWord() const
{
    auto format = opengl::ConvertToGLType(data_.pixel_structure());
    auto type = opengl::ConvertToGLType(data_.pixel_element_size());
    if (type != GL_UNSIGNED_INT)
    {
        throw std::runtime_error(
            "Invalid format should be unsigned int is : " +
            proto::PixelElementSize_Enum_Name(
                data_.pixel_element_size().value()));
    }
    auto pixel_structure = data_.pixel_structure().value();
    // 6 because of cubemap!
    std::size_t image_size = static_cast<std::size_t>(inner_size_.x) *
                             static_cast<std::size_t>(inner_size_.y) *
                             static_cast<std::size_t>(pixel_structure) * 6;
    std::vector<std::uint32_t> result = {};
    result.resize(image_size);
    glGetTextureImage(
        texture_id_,
        0,
        format,
        type,
        static_cast<GLsizei>(image_size * sizeof(std::uint32_t)),
        result.data());
    return result;
}

std::vector<float> Cubemap::GetTextureFloat() const
{
    auto format = opengl::ConvertToGLType(data_.pixel_structure());
    auto type = opengl::ConvertToGLType(data_.pixel_element_size());
    if (type != GL_FLOAT)
    {
        throw std::runtime_error(
            "Invalid format should be float is : " +
            proto::PixelElementSize_Enum_Name(
                data_.pixel_element_size().value()));
    }
    auto pixel_structure = data_.pixel_structure().value();
    // 6 because of cubemap!
    std::size_t image_size = static_cast<std::size_t>(inner_size_.x) *
                             static_cast<std::size_t>(inner_size_.y) *
                             static_cast<std::size_t>(pixel_structure) * 6;
    std::vector<float> result = {};
    result.resize(image_size);
    glGetTextureImage(
        texture_id_,
        0,
        format,
        type,
        static_cast<GLsizei>(image_size * sizeof(float)),
        result.data());
    return result;
}

void Cubemap::Update(
    std::vector<std::uint8_t>&& vector,
    glm::uvec2 size,
    std::uint8_t bytes_per_pixel)
{
    throw std::runtime_error("Not implemented yet!");
}

void Cubemap::EnableMipmap()
{
    data_.set_mipmap(true);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

glm::uvec2 Cubemap::GetSize()
{
    return inner_size_;
}

void Cubemap::SetDisplaySize(glm::uvec2 display_size)
{
    auto compute = [&](int stored, unsigned int display) {
        if (stored < 0)
        {
            return display / static_cast<unsigned int>(std::abs(stored));
        }
        else if (stored > 0)
        {
            return static_cast<unsigned int>(stored);
        }
        else
        {
            return display;
        }
    };

    inner_size_.x = compute(data_.size().x(), display_size.x);
    inner_size_.y = compute(data_.size().y(), display_size.y);
}

proto::TextureFrame Cubemap::GetTextureFrameFromPosition(int i)
{
    proto::TextureFrame texture_frame{};
    switch (i)
    {
    case 0:
        texture_frame.set_value(proto::TextureFrame::CUBE_MAP_POSITIVE_X);
        break;
    case 1:
        texture_frame.set_value(proto::TextureFrame::CUBE_MAP_NEGATIVE_X);
        break;
    case 2:
        texture_frame.set_value(proto::TextureFrame::CUBE_MAP_POSITIVE_Y);
        break;
    case 3:
        texture_frame.set_value(proto::TextureFrame::CUBE_MAP_NEGATIVE_Y);
        break;
    case 4:
        texture_frame.set_value(proto::TextureFrame::CUBE_MAP_POSITIVE_Z);
        break;
    case 5:
        texture_frame.set_value(proto::TextureFrame::CUBE_MAP_NEGATIVE_Z);
        break;
    default:
        throw std::runtime_error(std::format("Invalid entry {}.", i));
    }
    return texture_frame;
}

} // End namespace frame::opengl.


