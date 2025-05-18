#include "frame/opengl/cubemap.h"

#include <GL/glew.h>

#include <algorithm>
#include <cassert>
#include <functional>
#include <stdexcept>

#include "frame/json/parse_uniform.h"
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

proto::TextureFrame GetTextureFrameFromPosition(int i)
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
        throw std::runtime_error(fmt::format("Invalid entry {}.", i));
    }
    return texture_frame;
}

Cubemap::~Cubemap()
{
    glDeleteTextures(1, &texture_id_);
}

Cubemap::Cubemap(const TextureParameter& texture_parameter)
    : Cubemap(
          texture_parameter.pixel_element_size,
          texture_parameter.pixel_structure)
{
    texture_parameter_ = texture_parameter;
    assert(texture_parameter.map_type == TextureTypeEnum::CUBMAP);
    size_ = texture_parameter.size;
    CreateCubemap(texture_parameter.array_data_ptr);
}

void Cubemap::Bind(unsigned int slot /*= 0*/) const
{
    assert(slot < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
    if (locked_bind_)
        return;
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id_);
}

void Cubemap::UnBind() const
{
    if (locked_bind_)
        return;
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void Cubemap::CreateCubemap(
        const std::array<void*, 6> cube_map/* =
            { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }*/)
{
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
            opengl::ConvertToGLType(pixel_element_size_, pixel_structure_),
            static_cast<GLsizei>(size_.x),
            static_cast<GLsizei>(size_.y),
            0,
            opengl::ConvertToGLType(pixel_structure_),
            opengl::ConvertToGLType(pixel_element_size_),
            cube_map[i]);
    }
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
    render_->CreateStorage(size_);
    frame_->AttachRender(*render_);
    frame_->AttachTexture(GetId());
    frame_->DrawBuffers(1);
}

void Cubemap::Clear(glm::vec4 color)
{
    // First time this is called this will create a frame and a render.
    Bind();
    if (!frame_)
        CreateFrameAndRenderBuffer();
    ScopedBind scoped_frame(*frame_);
    glViewport(0, 0, size_.x, size_.y);
    GLfloat clear_color[4] = {color.r, color.g, color.b, color.a};
    glClearBufferfv(GL_COLOR, 0, clear_color);
    UnBind();
}

std::vector<std::uint8_t> Cubemap::GetTextureByte() const
{
    auto format = opengl::ConvertToGLType(pixel_structure_);
    auto type = opengl::ConvertToGLType(pixel_element_size_);
    if (type != GL_UNSIGNED_BYTE)
    {
        throw std::runtime_error(
            "Invalid format should be unsigned byte is : " +
            proto::PixelElementSize_Enum_Name(pixel_element_size_.value()));
    }
    auto size = json::ParseSize(data_.size());
    auto pixel_structure = data_.pixel_structure().value();
    // 6 because of cubemap!
    std::size_t image_size = static_cast<std::size_t>(size.x) *
                             static_cast<std::size_t>(size.y) *
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
    auto format = opengl::ConvertToGLType(pixel_structure_);
    auto type = opengl::ConvertToGLType(pixel_element_size_);
    if (type != GL_UNSIGNED_SHORT)
    {
        throw std::runtime_error(
            "Invalid format should be unsigned short is : " +
            proto::PixelElementSize_Enum_Name(pixel_element_size_.value()));
    }
    auto size = json::ParseSize(data_.size());
    auto pixel_structure = data_.pixel_structure().value();
    // 6 because of cubemap!
    std::size_t image_size = static_cast<std::size_t>(size.x) *
                             static_cast<std::size_t>(size.y) *
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
    auto format = opengl::ConvertToGLType(pixel_structure_);
    auto type = opengl::ConvertToGLType(pixel_element_size_);
    if (type != GL_UNSIGNED_INT)
    {
        throw std::runtime_error(
            "Invalid format should be unsigned int is : " +
            proto::PixelElementSize_Enum_Name(pixel_element_size_.value()));
    }
    auto size = json::ParseSize(data_.size());
    auto pixel_structure = data_.pixel_structure().value();
    // 6 because of cubemap!
    std::size_t image_size = static_cast<std::size_t>(size.x) *
                             static_cast<std::size_t>(size.y) *
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
    auto format = opengl::ConvertToGLType(pixel_structure_);
    auto type = opengl::ConvertToGLType(pixel_element_size_);
    if (type != GL_FLOAT)
    {
        throw std::runtime_error(
            "Invalid format should be float is : " +
            proto::PixelElementSize_Enum_Name(pixel_element_size_.value()));
    }
    auto size = json::ParseSize(data_.size());
    auto pixel_structure = data_.pixel_structure().value();
    // 6 because of cubemap!
    std::size_t image_size = static_cast<std::size_t>(size.x) *
                             static_cast<std::size_t>(size.y) *
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

} // End namespace frame::opengl.
