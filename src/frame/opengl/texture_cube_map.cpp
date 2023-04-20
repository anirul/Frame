#include "frame/opengl/texture_cube_map.h"

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

proto::TextureFrame GetTextureFrameFromPosition(int i) {
    proto::TextureFrame texture_frame{};
    switch (i) {
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

TextureCubeMap::~TextureCubeMap() { glDeleteTextures(1, &texture_id_); }

TextureCubeMap::TextureCubeMap(const TextureParameter& texture_parameter)
    : TextureCubeMap(texture_parameter.pixel_element_size, texture_parameter.pixel_structure) {
    assert(texture_parameter.map_type == TextureTypeEnum::CUBMAP);
    size_ = texture_parameter.size;
    CreateTextureCubeMap(texture_parameter.array_data_ptr);
}

void TextureCubeMap::Bind(const unsigned int slot /*= 0*/) const {
    assert(slot < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
    if (locked_bind_) return;
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id_);
}

void TextureCubeMap::UnBind() const {
    if (locked_bind_) return;
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void TextureCubeMap::EnableMipmap() const { glGenerateMipmap(GL_TEXTURE_CUBE_MAP); }

void TextureCubeMap::SetMinFilter(const proto::TextureFilter::Enum texture_filter) {
    Bind();
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, ConvertToGLType(texture_filter));
    UnBind();
}

frame::proto::TextureFilter::Enum TextureCubeMap::GetMinFilter() const {
    GLint filter;
    Bind();
    glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, &filter);
    UnBind();
    return ConvertFromGLType(filter);
}

void TextureCubeMap::SetMagFilter(const proto::TextureFilter::Enum texture_filter) {
    Bind();
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, ConvertToGLType(texture_filter));
    UnBind();
}

frame::proto::TextureFilter::Enum TextureCubeMap::GetMagFilter() const {
    GLint filter;
    Bind();
    glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, &filter);
    UnBind();
    return ConvertFromGLType(filter);
}

void TextureCubeMap::SetWrapS(const proto::TextureFilter::Enum texture_filter) {
    Bind();
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, ConvertToGLType(texture_filter));
    UnBind();
}

frame::proto::TextureFilter::Enum TextureCubeMap::GetWrapS() const {
    GLint filter;
    Bind();
    glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, &filter);
    UnBind();
    return ConvertFromGLType(filter);
}

void TextureCubeMap::SetWrapT(const proto::TextureFilter::Enum texture_filter) {
    Bind();
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, ConvertToGLType(texture_filter));
    UnBind();
}

frame::proto::TextureFilter::Enum TextureCubeMap::GetWrapT() const {
    GLint filter;
    Bind();
    glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, &filter);
    UnBind();
    return ConvertFromGLType(filter);
}

void TextureCubeMap::SetWrapR(const proto::TextureFilter::Enum texture_filter) {
    Bind();
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, ConvertToGLType(texture_filter));
    UnBind();
}

proto::TextureFilter::Enum TextureCubeMap::GetWrapR() const {
    GLint filter;
    Bind();
    glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, &filter);
    UnBind();
    return ConvertFromGLType(filter);
}

void TextureCubeMap::CreateTextureCubeMap(
		const std::array<void*, 6> cube_map/* =
			{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }*/)
	{
    glGenTextures(1, &texture_id_);
    ScopedBind scoped_bind(*this);
    SetMinFilter(proto::TextureFilter::LINEAR);
    SetMagFilter(proto::TextureFilter::LINEAR);
    SetWrapS(proto::TextureFilter::CLAMP_TO_EDGE);
    SetWrapT(proto::TextureFilter::CLAMP_TO_EDGE);
    SetWrapR(proto::TextureFilter::CLAMP_TO_EDGE);
    for (unsigned int i : { 0, 1, 2, 3, 4, 5 }) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
                     opengl::ConvertToGLType(pixel_element_size_, pixel_structure_),
                     static_cast<GLsizei>(size_.x), static_cast<GLsizei>(size_.y), 0,
                     opengl::ConvertToGLType(pixel_structure_),
                     opengl::ConvertToGLType(pixel_element_size_), cube_map[i]);
    }
}

int TextureCubeMap::ConvertToGLType(const proto::TextureFilter::Enum texture_filter) const {
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
        default:
            throw std::runtime_error("Invalid texture filter : " +
                                     std::to_string(static_cast<int>(texture_filter)));
    }
}

frame::proto::TextureFilter::Enum TextureCubeMap::ConvertFromGLType(int gl_filter) const {
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

void TextureCubeMap::CreateFrameAndRenderBuffer() {
    frame_  = std::make_unique<FrameBuffer>();
    render_ = std::make_unique<RenderBuffer>();
    render_->CreateStorage(size_);
    frame_->AttachRender(*render_);
    frame_->AttachTexture(GetId());
    frame_->DrawBuffers(1);
}

void TextureCubeMap::Clear(const glm::vec4 color) {
    // First time this is called this will create a frame and a render.
    Bind();
    if (!frame_) CreateFrameAndRenderBuffer();
    ScopedBind scoped_frame(*frame_);
    glViewport(0, 0, size_.x, size_.y);
    GLfloat clear_color[4] = { color.r, color.g, color.b, color.a };
    glClearBufferfv(GL_COLOR, 0, clear_color);
    UnBind();
}

std::vector<std::uint8_t> TextureCubeMap::GetTextureByte() const {
    auto format = opengl::ConvertToGLType(pixel_structure_);
    auto type   = opengl::ConvertToGLType(pixel_element_size_);
    if (type != GL_UNSIGNED_BYTE) {
        throw std::runtime_error("Invalid format should be unsigned byte is : " +
                                 proto::PixelElementSize_Enum_Name(pixel_element_size_.value()));
    }
    auto size            = GetSize();
    auto pixel_structure = GetPixelStructure();
    // 6 because of cubemap!
    std::size_t image_size = static_cast<std::size_t>(size.x) * static_cast<std::size_t>(size.y) *
                             static_cast<std::size_t>(pixel_structure) * 6;
    std::vector<std::uint8_t> result = {};
    result.resize(image_size);
    glGetTextureImage(texture_id_, 0, format, type,
                      static_cast<GLsizei>(image_size * sizeof(std::uint8_t)), result.data());
    return result;
}

std::vector<std::uint16_t> TextureCubeMap::GetTextureWord() const {
    auto format = opengl::ConvertToGLType(pixel_structure_);
    auto type   = opengl::ConvertToGLType(pixel_element_size_);
    if (type != GL_UNSIGNED_SHORT) {
        throw std::runtime_error("Invalid format should be unsigned short is : " +
                                 proto::PixelElementSize_Enum_Name(pixel_element_size_.value()));
    }
    auto size            = GetSize();
    auto pixel_structure = GetPixelStructure();
    // 6 because of cubemap!
    std::size_t image_size = static_cast<std::size_t>(size.x) * static_cast<std::size_t>(size.y) *
                             static_cast<std::size_t>(pixel_structure) * 6;
    std::vector<std::uint16_t> result = {};
    result.resize(image_size);
    glGetTextureImage(texture_id_, 0, format, type,
                      static_cast<GLsizei>(image_size * sizeof(std::uint16_t)), result.data());
    return result;
}

std::vector<std::uint32_t> TextureCubeMap::GetTextureDWord() const {
    auto format = opengl::ConvertToGLType(pixel_structure_);
    auto type   = opengl::ConvertToGLType(pixel_element_size_);
    if (type != GL_UNSIGNED_INT) {
        throw std::runtime_error("Invalid format should be unsigned int is : " +
                                 proto::PixelElementSize_Enum_Name(pixel_element_size_.value()));
    }
    auto size            = GetSize();
    auto pixel_structure = GetPixelStructure();
    // 6 because of cubemap!
    std::size_t image_size = static_cast<std::size_t>(size.x) * static_cast<std::size_t>(size.y) *
                             static_cast<std::size_t>(pixel_structure) * 6;
    std::vector<std::uint32_t> result = {};
    result.resize(image_size);
    glGetTextureImage(texture_id_, 0, format, type,
                      static_cast<GLsizei>(image_size * sizeof(std::uint32_t)), result.data());
    return result;
}

std::vector<float> TextureCubeMap::GetTextureFloat() const {
    auto format = opengl::ConvertToGLType(pixel_structure_);
    auto type   = opengl::ConvertToGLType(pixel_element_size_);
    if (type != GL_FLOAT) {
        throw std::runtime_error("Invalid format should be float is : " +
                                 proto::PixelElementSize_Enum_Name(pixel_element_size_.value()));
    }
    auto size            = GetSize();
    auto pixel_structure = GetPixelStructure();
    // 6 because of cubemap!
    std::size_t image_size = static_cast<std::size_t>(size.x) * static_cast<std::size_t>(size.y) *
                             static_cast<std::size_t>(pixel_structure) * 6;
    std::vector<float> result = {};
    result.resize(image_size);
    glGetTextureImage(texture_id_, 0, format, type,
                      static_cast<GLsizei>(image_size * sizeof(float)), result.data());
    return result;
}

void TextureCubeMap::Update(std::vector<std::uint8_t>&& vector, glm::uvec2 size,
                            std::uint8_t bytes_per_pixel) {
    throw std::runtime_error("Not implemented yet!");
}

}  // End namespace frame::opengl.
