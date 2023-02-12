#include "frame/opengl/texture.h"

#include <GL/glew.h>

#include <algorithm>
#include <cassert>
#include <functional>
#include <stdexcept>

#include "frame/level.h"
#include "frame/opengl/file/load_program.h"
#include "frame/opengl/frame_buffer.h"
#include "frame/opengl/pixel.h"
#include "frame/opengl/program.h"
#include "frame/opengl/render_buffer.h"
#include "frame/opengl/renderer.h"
#include "frame/opengl/static_mesh.h"

namespace frame::opengl {

Texture::Texture(const TextureParameter& texture_parameter)
    : size_(texture_parameter.size),
      pixel_element_size_(texture_parameter.pixel_element_size),
      pixel_structure_(texture_parameter.pixel_structure) {
    assert(texture_parameter.map_type == TextureTypeEnum::TEXTURE_2D);
    CreateTexture(texture_parameter.data_ptr);
}

void Texture::CreateTexture(const void* data /* = nullptr*/) {
    glGenTextures(1, &texture_id_);
    ScopedBind scoped_bind(*this);
    SetMinFilter(proto::TextureFilter::LINEAR);
    SetMagFilter(proto::TextureFilter::LINEAR);
    SetWrapS(proto::TextureFilter::CLAMP_TO_EDGE);
    SetWrapT(proto::TextureFilter::CLAMP_TO_EDGE);
    auto format = opengl::ConvertToGLType(pixel_structure_);
    auto type   = opengl::ConvertToGLType(pixel_element_size_);
    glTexImage2D(GL_TEXTURE_2D, 0, opengl::ConvertToGLType(pixel_element_size_, pixel_structure_),
                 static_cast<GLsizei>(size_.x), static_cast<GLsizei>(size_.y), 0, format, type,
                 data);
}

Texture::~Texture() { glDeleteTextures(1, &texture_id_); }

void Texture::Bind(const unsigned int slot /*= 0*/) const {
    if (locked_bind_) return;
    assert(slot < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
}

void Texture::UnBind() const {
    if (locked_bind_) return;
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::EnableMipmap() const { glGenerateMipmap(GL_TEXTURE_2D); }

void Texture::SetMinFilter(const proto::TextureFilter::Enum texture_filter) {
    Bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ConvertToGLType(texture_filter));
    UnBind();
}

proto::TextureFilter::Enum Texture::GetMinFilter() const {
    GLint filter;
    Bind();
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &filter);
    UnBind();
    return ConvertFromGLType(filter);
}

void Texture::SetMagFilter(const proto::TextureFilter::Enum texture_filter) {
    Bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ConvertToGLType(texture_filter));
    UnBind();
}

proto::TextureFilter::Enum Texture::GetMagFilter() const {
    GLint filter;
    Bind();
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &filter);
    UnBind();
    return ConvertFromGLType(filter);
}

void Texture::SetWrapS(const proto::TextureFilter::Enum texture_filter) {
    Bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, ConvertToGLType(texture_filter));
    UnBind();
}

proto::TextureFilter::Enum Texture::GetWrapS() const {
    GLint filter;
    Bind();
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &filter);
    UnBind();
    return ConvertFromGLType(filter);
}

void Texture::SetWrapT(const proto::TextureFilter::Enum texture_filter) {
    Bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, ConvertToGLType(texture_filter));
    UnBind();
}

proto::TextureFilter::Enum Texture::GetWrapT() const {
    GLint filter;
    Bind();
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &filter);
    UnBind();
    return ConvertFromGLType(filter);
}

void Texture::CreateFrameAndRenderBuffer() {
    frame_  = std::make_unique<FrameBuffer>();
    render_ = std::make_unique<RenderBuffer>();
    render_->CreateStorage(size_);
    frame_->AttachRender(*render_);
    frame_->AttachTexture(GetId());
    frame_->DrawBuffers(1);
}

void Texture::Clear(const glm::vec4 color) {
    // First time this is called this will create a frame and a render.
    Bind();
    if (!frame_) CreateFrameAndRenderBuffer();
    ScopedBind scoped_frame(*frame_);
    glViewport(0, 0, size_.x, size_.y);
    GLfloat clear_color[4] = { color.r, color.g, color.b, color.a };
    glClearBufferfv(GL_COLOR, 0, clear_color);
    UnBind();
}

int Texture::ConvertToGLType(const proto::TextureFilter::Enum texture_filter) const {
    switch (texture_filter) {
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
            throw std::runtime_error("Invalid texture filter : " +
                                     std::to_string(static_cast<int>(texture_filter)));
    }
}

frame::proto::TextureFilter::Enum Texture::ConvertFromGLType(int gl_filter) const {
    switch (gl_filter) {
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
    throw std::runtime_error("invalid texture filter : " + std::to_string(gl_filter));
}

std::vector<std::uint8_t> Texture::GetTextureByte() const {
    Bind();
    auto format = opengl::ConvertToGLType(pixel_structure_);
    auto type   = opengl::ConvertToGLType(pixel_element_size_);
    if (type != GL_UNSIGNED_BYTE) {
        throw std::runtime_error("Invalid format should be byte is : " +
                                 proto::PixelElementSize_Enum_Name(pixel_element_size_.value()));
    }
    auto size              = GetSize();
    auto pixel_structure   = GetPixelStructure();
    std::size_t image_size = static_cast<std::size_t>(size.x) * static_cast<std::size_t>(size.y) *
                             static_cast<std::size_t>(pixel_structure);
    std::vector<std::uint8_t> result = {};
    result.resize(image_size);
    glGetTexImage(GL_TEXTURE_2D, 0, format, type, result.data());
    UnBind();
    return result;
}

std::vector<std::uint16_t> Texture::GetTextureWord() const {
    Bind();
    auto format = opengl::ConvertToGLType(pixel_structure_);
    auto type   = opengl::ConvertToGLType(pixel_element_size_);
    if (type != GL_UNSIGNED_SHORT) {
        throw std::runtime_error("Invalid format should be float is : " +
                                 proto::PixelElementSize_Enum_Name(pixel_element_size_.value()));
    }
    auto size              = GetSize();
    auto pixel_structure   = GetPixelStructure();
    std::size_t image_size = static_cast<std::size_t>(size.x) * static_cast<std::size_t>(size.y) *
                             static_cast<std::size_t>(pixel_structure);
    std::vector<std::uint16_t> result = {};
    result.resize(image_size);
    glGetTexImage(GL_TEXTURE_2D, 0, format, type, result.data());
    UnBind();
    return result;
}

std::vector<std::uint32_t> Texture::GetTextureDWord() const {
    Bind();
    auto format = opengl::ConvertToGLType(pixel_structure_);
    auto type   = opengl::ConvertToGLType(pixel_element_size_);
    if (type != GL_UNSIGNED_INT) {
        throw std::runtime_error("Invalid format should be float is : " +
                                 proto::PixelElementSize_Enum_Name(pixel_element_size_.value()));
    }
    auto size              = GetSize();
    auto pixel_structure   = GetPixelStructure();
    std::size_t image_size = static_cast<std::size_t>(size.x) * static_cast<std::size_t>(size.y) *
                             static_cast<std::size_t>(pixel_structure);
    std::vector<std::uint32_t> result = {};
    result.resize(image_size);
    glGetTexImage(GL_TEXTURE_2D, 0, format, type, result.data());
    UnBind();
    return result;
}

std::vector<float> Texture::GetTextureFloat() const {
    Bind();
    auto format = opengl::ConvertToGLType(pixel_structure_);
    auto type   = opengl::ConvertToGLType(pixel_element_size_);
    if (type != GL_FLOAT) {
        throw std::runtime_error("Invalid format should be float is : " +
                                 proto::PixelElementSize_Enum_Name(pixel_element_size_.value()));
    }
    auto size              = GetSize();
    auto pixel_structure   = GetPixelStructure();
    std::size_t image_size = static_cast<std::size_t>(size.x) * static_cast<std::size_t>(size.y) *
                             static_cast<std::size_t>(pixel_structure);
    std::vector<float> result = {};
    result.resize(image_size);
    glGetTexImage(GL_TEXTURE_2D, 0, format, type, result.data());
    UnBind();
    return result;
}

void Texture::Update(std::vector<std::uint8_t>&& vector, glm::uvec2 size,
                     std::uint8_t bytes_per_pixel) {
    ScopedBind scoped_bind(*this);
    size_ = size;
    assert(pixel_element_size_.value() == 1);
    auto format = opengl::ConvertToGLType(pixel_structure_);
    auto type   = opengl::ConvertToGLType(pixel_element_size_);
    glTexImage2D(GL_TEXTURE_2D, 0, opengl::ConvertToGLType(pixel_element_size_, pixel_structure_),
                 static_cast<GLsizei>(size_.x), static_cast<GLsizei>(size_.y), 0, format, type,
                 vector.data());
}

}  // End namespace frame::opengl.
