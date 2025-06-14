#include "frame/opengl/texture.h"

#include <GL/glew.h>
#include <algorithm>
#include <cassert>
#include <functional>
#include <google/protobuf/descriptor.h>
#include <stdexcept>

#include "frame/file/file_system.h"
#include "frame/file/image.h"
#include "frame/json/parse_uniform.h"
#include "frame/json/serialize_uniform.h"
#include "frame/level.h"
#include "frame/opengl/file/load_program.h"
#include "frame/opengl/frame_buffer.h"
#include "frame/opengl/pixel.h"
#include "frame/opengl/program.h"
#include "frame/opengl/render_buffer.h"
#include "frame/opengl/renderer.h"
#include "frame/opengl/static_mesh.h"

namespace frame::opengl
{

Texture::Texture(const proto::Texture& proto_texture, glm::uvec2 display_size)
{
    data_ = proto_texture;
    std::string name = proto_texture.name();
    SetDisplaySize(display_size);
    switch (proto_texture.texture_oneof_case())
    {
    case proto::Texture::kPixels:
        CreateTextureFromPointer(
            data_.pixels().data(),
            GetSize(),
            data_.pixel_element_size(),
            data_.pixel_structure());
        break;
    case proto::Texture::kFileName:
        if (proto_texture.file_name().empty())
        {
            // Empty path means no initial data, just allocate storage.
            CreateTextureFromPointer(
                nullptr,
                GetSize(),
                data_.pixel_element_size(),
                data_.pixel_structure());
        }
        else
        {
            CreateTextureFromFile(
                data_.file_name(),
                data_.pixel_element_size(),
                data_.pixel_structure());
        }
        break;
    case proto::Texture::kPlugin:
        throw std::runtime_error("Don't know what to do there?");
    case proto::Texture::kFileNames:
        throw std::runtime_error("This is a Texture not a cubemap?");
    case 0:
        CreateTextureFromPointer(
            nullptr,
            GetSize(),
            data_.pixel_element_size(),
            data_.pixel_structure());
        break;
    default: {
        const auto enum_value = proto_texture.texture_oneof_case();
        const auto* value_desc =
            proto_texture.GetDescriptor()->FindFieldByNumber(enum_value);
        throw std::runtime_error(
            std::format(
                "Unknown texture [{}] type [{}]?",
                name,
                value_desc ? value_desc->name() : "<unknown>"));
    }
    }
}

Texture::Texture(
    std::filesystem::path file_name,
    proto::PixelElementSize pixel_element_size,
    proto::PixelStructure pixel_structure)
{
    CreateTextureFromFile(file_name, pixel_element_size, pixel_structure);
}

Texture::Texture(
    const void* ptr,
    glm::uvec2 size,
    proto::PixelElementSize pixel_element_size,
    proto::PixelStructure pixel_structure)
{
    CreateTextureFromPointer(ptr, size, pixel_element_size, pixel_structure);
}

void Texture::CreateTextureFromFile(
    std::filesystem::path file_name,
    proto::PixelElementSize pixel_element_size,
    proto::PixelStructure pixel_structure)
{
    if (file_name.empty())
    {
        // No file provided: allocate empty texture using existing size data.
        CreateTextureFromPointer(
            nullptr, GetSize(), pixel_element_size, pixel_structure);
        return;
    }
    data_.mutable_pixel_element_size()->CopyFrom(pixel_element_size);
    data_.mutable_pixel_structure()->CopyFrom(pixel_structure);
    data_.set_file_name(frame::file::PurifyFilePath(file_name));
    frame::file::Image image(
        frame::file::FindFile(file_name),
        data_.pixel_element_size(),
        data_.pixel_structure());
    SetDisplaySize(image.GetSize());
    CreateTextureFromPointer(
        image.Data(), image.GetSize(), pixel_element_size, pixel_structure);
}

void Texture::CreateTextureFromPointer(
    const void* data,
    glm::uvec2 size,
    proto::PixelElementSize pixel_element_size,
    proto::PixelStructure pixel_structure)
{
    data_.mutable_pixel_element_size()->CopyFrom(pixel_element_size);
    data_.mutable_pixel_structure()->CopyFrom(pixel_structure);
    if (!size.x || !size.y)
    {
        inner_size_ = json::ParseSize(data_.size());
    }
    else
    {
        inner_size_ = size;
        if (!data_.has_size())
            data_.mutable_size()->CopyFrom(json::SerializeSize(inner_size_));
    }
    glCreateTextures(GL_TEXTURE_2D, 1, &texture_id_);
    proto::TextureFilter texture_filter;
    texture_filter.set_value(proto::TextureFilter::LINEAR);
    glTextureParameteri(
        texture_id_,
        GL_TEXTURE_MIN_FILTER,
        ConvertToGLType(proto::TextureFilter::LINEAR));
    data_.mutable_min_filter()->CopyFrom(texture_filter);
    glTextureParameteri(
        texture_id_,
        GL_TEXTURE_MAG_FILTER,
        ConvertToGLType(proto::TextureFilter::LINEAR));
    data_.mutable_mag_filter()->CopyFrom(texture_filter);
    texture_filter.set_value(proto::TextureFilter::CLAMP_TO_EDGE);
    glTextureParameteri(
        texture_id_,
        GL_TEXTURE_WRAP_S,
        ConvertToGLType(proto::TextureFilter::CLAMP_TO_EDGE));
    data_.mutable_wrap_s()->CopyFrom(texture_filter);
    glTextureParameteri(
        texture_id_,
        GL_TEXTURE_WRAP_T,
        ConvertToGLType(proto::TextureFilter::CLAMP_TO_EDGE));
    data_.mutable_wrap_t()->CopyFrom(texture_filter);
    const GLenum internal_format = opengl::ConvertToGLType(
        data_.pixel_element_size(), data_.pixel_structure());
    const GLenum format = opengl::ConvertToGLType(data_.pixel_structure());
    const GLenum type = opengl::ConvertToGLType(data_.pixel_element_size());
    glTextureStorage2D(
        texture_id_,
        1,
        internal_format,
        static_cast<GLsizei>(inner_size_.x),
        static_cast<GLsizei>(inner_size_.y));
    if (data)
    {
        GLint previous_align = 0;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &previous_align);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTextureSubImage2D(
            texture_id_,
            0,
            0,
            0,
            static_cast<GLsizei>(inner_size_.x),
            static_cast<GLsizei>(inner_size_.y),
            format,
            type,
            data);
        glPixelStorei(GL_UNPACK_ALIGNMENT, previous_align);
    }
}

void Texture::CreateDepthTexture(
    glm::uvec2 size,
    proto::PixelElementSize pixel_element_size /* =
        proto::PixelElementSize_FLOAT()*/)
{
    inner_size_ = size;
    if (!data_.has_size())
        data_.mutable_size()->CopyFrom(json::SerializeSize(inner_size_));
    glCreateTextures(GL_TEXTURE_2D, 1, &texture_id_);
    proto::TextureFilter texture_filter;
    texture_filter.set_value(proto::TextureFilter::NEAREST);
    glTextureParameteri(
        texture_id_,
        GL_TEXTURE_MIN_FILTER,
        ConvertToGLType(proto::TextureFilter::NEAREST));
    data_.mutable_min_filter()->CopyFrom(texture_filter);
    glTextureParameteri(
        texture_id_,
        GL_TEXTURE_MAG_FILTER,
        ConvertToGLType(proto::TextureFilter::NEAREST));
    data_.mutable_mag_filter()->CopyFrom(texture_filter);
    texture_filter.set_value(proto::TextureFilter::CLAMP_TO_EDGE);
    glTextureParameteri(
        texture_id_,
        GL_TEXTURE_WRAP_S,
        ConvertToGLType(proto::TextureFilter::CLAMP_TO_EDGE));
    data_.mutable_wrap_s()->CopyFrom(texture_filter);
    glTextureParameteri(
        texture_id_,
        GL_TEXTURE_WRAP_T,
        ConvertToGLType(proto::TextureFilter::CLAMP_TO_EDGE));
    data_.mutable_wrap_t()->CopyFrom(texture_filter);
    glTextureStorage2D(
        texture_id_,
        1,
        GL_DEPTH_COMPONENT32F,
        static_cast<GLsizei>(data_.size().x()),
        static_cast<GLsizei>(data_.size().y()));
}

Texture::~Texture()
{
    glDeleteTextures(1, &texture_id_);
}

void Texture::Bind(unsigned int slot /*= 0*/) const
{
    if (locked_bind_)
        return;
    assert(slot < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
}

void Texture::UnBind() const
{
    if (locked_bind_)
        return;
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::EnableMipmap()
{
    data_.set_mipmap(true);
    glGenerateMipmap(GL_TEXTURE_2D);
}

glm::uvec2 Texture::GetSize()
{
    return inner_size_;
}

void Texture::SetDisplaySize(glm::uvec2 display_size)
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

void Texture::CreateFrameAndRenderBuffer()
{
    frame_ = std::make_unique<FrameBuffer>();
    render_ = std::make_unique<RenderBuffer>();
    render_->CreateStorage(json::ParseSize(data_.size()));
    frame_->AttachRender(*render_);
    frame_->AttachTexture(GetId());
    frame_->DrawBuffers(1);
}

void Texture::Clear(glm::vec4 color)
{
    // First time this is called this will create a frame and a render.
    Bind();
    if (!frame_)
        CreateFrameAndRenderBuffer();
    ScopedBind scoped_frame(*frame_);
    glViewport(0, 0, inner_size_.x, inner_size_.y);
    GLfloat clear_color[4] = {color.r, color.g, color.b, color.a};
    glClearBufferfv(GL_COLOR, 0, clear_color);
    UnBind();
}

int Texture::ConvertToGLType(
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
    case frame::proto::TextureFilter::CLAMP_TO_BORDER:
        return GL_CLAMP_TO_BORDER;
    default:
        throw std::runtime_error(
            "Invalid texture filter : " +
            std::to_string(static_cast<int>(texture_filter)));
    }
}

frame::proto::TextureFilter::Enum Texture::ConvertFromGLType(
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

std::vector<std::uint8_t> Texture::GetTextureByte() const
{
    auto format = opengl::ConvertToGLType(data_.pixel_structure());
    auto type = opengl::ConvertToGLType(data_.pixel_element_size());
    if (type != GL_UNSIGNED_BYTE)
    {
        throw std::runtime_error(
            "Invalid format should be byte is : " +
            proto::PixelElementSize_Enum_Name(
                data_.pixel_element_size().value()));
    }
    auto pixel_structure = data_.pixel_structure().value();
    std::size_t image_size = static_cast<std::size_t>(inner_size_.x) *
                             static_cast<std::size_t>(inner_size_.y) *
                             static_cast<std::size_t>(pixel_structure);
    std::vector<std::uint8_t> result(image_size);
    glGetTextureImage(
        texture_id_,
        0,
        format,
        type,
        static_cast<GLsizei>(image_size * sizeof(std::uint8_t)),
        result.data());
    return result;
}

std::vector<std::uint16_t> Texture::GetTextureWord() const
{
    auto format = opengl::ConvertToGLType(data_.pixel_structure());
    auto type = opengl::ConvertToGLType(data_.pixel_element_size());
    if (type != GL_UNSIGNED_SHORT)
    {
        throw std::runtime_error(
            "Invalid format should be float is : " +
            proto::PixelElementSize_Enum_Name(
                data_.pixel_element_size().value()));
    }
    auto pixel_structure = data_.pixel_structure().value();
    std::size_t image_size = static_cast<std::size_t>(inner_size_.x) *
                             static_cast<std::size_t>(inner_size_.y) *
                             static_cast<std::size_t>(pixel_structure);
    std::vector<std::uint16_t> result(image_size);
    glGetTextureImage(
        texture_id_,
        0,
        format,
        type,
        static_cast<GLsizei>(image_size * sizeof(std::uint16_t)),
        result.data());
    return result;
}

std::vector<std::uint32_t> Texture::GetTextureDWord() const
{
    auto format = opengl::ConvertToGLType(data_.pixel_structure());
    auto type = opengl::ConvertToGLType(data_.pixel_element_size());
    if (type != GL_UNSIGNED_INT)
    {
        throw std::runtime_error(
            "Invalid format should be float is : " +
            proto::PixelElementSize_Enum_Name(
                data_.pixel_element_size().value()));
    }
    auto pixel_structure = data_.pixel_structure().value();
    std::size_t image_size = static_cast<std::size_t>(inner_size_.x) *
                             static_cast<std::size_t>(inner_size_.y) *
                             static_cast<std::size_t>(pixel_structure);
    std::vector<std::uint32_t> result(image_size);
    glGetTextureImage(
        texture_id_,
        0,
        format,
        type,
        static_cast<GLsizei>(image_size * sizeof(std::uint32_t)),
        result.data());
    return result;
}

std::vector<float> Texture::GetTextureFloat() const
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
    std::size_t image_size = static_cast<std::size_t>(inner_size_.x) *
                             static_cast<std::size_t>(inner_size_.y) *
                             static_cast<std::size_t>(pixel_structure);
    std::vector<float> result(image_size);
    glGetTextureImage(
        texture_id_,
        0,
        format,
        type,
        static_cast<GLsizei>(image_size * sizeof(float)),
        result.data());
    return result;
}

void Texture::Update(
    std::vector<std::uint8_t>&& vector,
    glm::uvec2 size,
    std::uint8_t bytes_per_pixel)
{
    assert(data_.pixel_element_size().value() == 1);
    auto format = opengl::ConvertToGLType(data_.pixel_structure());
    auto type = opengl::ConvertToGLType(data_.pixel_element_size());
    glTextureSubImage2D(
        texture_id_,
        0,
        0,
        0,
        static_cast<GLsizei>(inner_size_.x),
        static_cast<GLsizei>(inner_size_.y),
        format,
        type,
        vector.data());
}

} // End namespace frame::opengl.
