#include "frame/opengl/cubemap.h"

#include <GL/glew.h>

#include <algorithm>
#include <cassert>
#include <fstream>
#include <functional>
#include <stdexcept>

#include "frame/file/file_system.h"
#include "frame/file/image.h"
#include "frame/json/parse_level.h"
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

namespace
{
// Get the 6 view for the cube map.
const std::array<glm::mat4, 6> views_cubemap = {
    glm::lookAt(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(-1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)),
    glm::lookAt(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)),
    glm::lookAt(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)),
    glm::lookAt(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f)),
    glm::lookAt(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)),
    glm::lookAt(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(0.0f, 1.0f, 0.0f))};
// Projection cube map.
const glm::mat4 projection_cubemap =
    glm::perspective(glm::radians(90.0f), 1.0f, 0.01f, 10.0f);
const std::set<std::string> byte_extention = {"jpeg", "jpg"};
const std::set<std::string> rgba_extention = {"png"};
const std::set<std::string> half_extention = {"hdr", "dds"};

// Taken from cpp reference.
std::size_t ReplaceAll(
    std::string& inout,
    const std::string_view what,
    const std::string_view with)
{
    std::size_t count = 0;
    for (std::string::size_type pos = 0;
         inout.npos != (pos = inout.find(what.data(), pos, what.length()));
         pos += with.length(), ++count)
    {
        inout.replace(pos, what.length(), with.data(), with.length());
    }
    return count;
}

std::string FillLevel(
    const std::string& initial, const std::map<std::string, std::string>& map)
{
    std::string out = initial;
    for (auto [from, to] : map)
    {
        ReplaceAll(to, "\\", "/");
        ReplaceAll(out, from, to);
    }
    return out;
}

std::uint32_t PowerFloor(std::uint32_t x)
{
    std::uint32_t power = 1;
    while (x >>= 1)
    {
        power <<= 1;
    }
    return power;
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
    CreateCubemapFromFile(file_name, pixel_element_size, pixel_structure);
}

Cubemap::Cubemap(
    std::array<std::filesystem::path, 6> file_names,
    proto::PixelElementSize pixel_element_size,
    proto::PixelStructure pixel_structure)
{
    CreateCubemapFromFiles(file_names, pixel_element_size, pixel_structure);
}

Cubemap::Cubemap(
    const void* ptr,
    glm::uvec2 size,
    proto::PixelElementSize pixel_element_size,
    proto::PixelStructure pixel_structure)
{
    CreateCubemapFromPointer(ptr, size, pixel_element_size, pixel_structure);
}

Cubemap::Cubemap(
    std::array<const void*, 6> ptrs,
    glm::uvec2 size,
    proto::PixelElementSize pixel_element_size,
    proto::PixelStructure pixel_structure)
{
    CreateCubemapFromPointers(ptrs, size, pixel_element_size, pixel_structure);
}

Cubemap::Cubemap(const proto::Texture& proto_texture, glm::uvec2 display_size)
{
    data_ = proto_texture;
    SetDisplaySize(display_size);
    switch (proto_texture.texture_oneof_case())
    {
    case proto::Texture::kPlugin:
        [[fallthrough]];
    case proto::Texture::kPixels:
        throw std::runtime_error("Not supported yet.");
    case proto::Texture::kFileName:
        CreateCubemapFromFile(
            proto_texture.file_name(),
            proto_texture.pixel_element_size(),
            proto_texture.pixel_structure());
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
            display_size,
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
    data_.mutable_pixel_element_size()->CopyFrom(pixel_element_size);
    data_.mutable_pixel_structure()->CopyFrom(pixel_structure);
    data_.set_file_name(frame::file::PurifyFilePath(file_name));
    frame::file::Image image(
        frame::file::FindFile(file_name),
        data_.pixel_element_size(),
        data_.pixel_structure());
    data_.mutable_size()->CopyFrom(json::SerializeSize(image.GetSize()));
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
    data_.mutable_pixel_element_size()->CopyFrom(pixel_element_size);
    data_.mutable_pixel_structure()->CopyFrom(pixel_structure);
    std::vector<std::unique_ptr<frame::file::Image>> images;
    std::vector<std::filesystem::path> paths;
    for (const std::filesystem::path& file_name : file_names)
    {
        paths.push_back(file_name);
        auto image_ptr = std::make_unique<frame::file::Image>(
            frame::file::FindFile(file_name));
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
    data_.mutable_size()->CopyFrom(json::SerializeSize(images[0]->GetSize()));
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
    data_.mutable_pixel_element_size()->CopyFrom(pixel_element_size);
    data_.mutable_pixel_structure()->CopyFrom(pixel_structure);
    data_.mutable_size()->CopyFrom(json::SerializeSize(size));
    auto& logger = Logger::GetInstance();
    std::unique_ptr<TextureInterface> equirectangular =
        std::make_unique<Texture>(
            ptr, size, pixel_element_size, pixel_structure);
    if (!equirectangular)
    {
        throw std::runtime_error("Couldn't load cubemap from single ptr.");
    }
    auto equirectangular_size = equirectangular->GetSize();
    // Seams correct when you are less than 2048 in height you get 512.
    std::uint32_t cube_single_res = PowerFloor(size.y);
    glm::uvec2 cube_pair_res = {cube_single_res, cube_single_res};
    std::map<std::string, std::string> filling_map = {
        {"<filename>", data_.file_name()},
        {"<x>", std::to_string(cube_pair_res.x)},
        {"<y>", std::to_string(cube_pair_res.y)},
        {"<pixel_element_size>",
         PixelElementSize_Enum_Name(pixel_element_size.value())},
        {"<pixel_structure>",
         PixelStructure_Enum_Name(pixel_structure.value())}};
    // Now get it from external file.
    // FIXME(anirul): Should make it a const string somewhere.
    std::ifstream ifs(
        frame::file::FindFile("asset/json/equirectangular.json").string());
    std::string inner_file_json((std::istreambuf_iterator<char>(ifs)), {});
    auto level = json::ParseLevel(
        cube_pair_res, FillLevel(inner_file_json, filling_map));
    if (!level)
    {
        throw std::runtime_error("Could not create level.");
    }
    auto out_texture_id = level->GetIdFromName("OutputTexture");
    if (out_texture_id == NullId)
    {
        throw std::runtime_error("Could not get the id of \"OutputTexture\".");
    }
    auto& out_texture_ref = level->GetTextureFromId(out_texture_id);
    Renderer renderer(*level.get(), {0, 0, cube_pair_res.x, cube_pair_res.y});
    auto& mesh_ref =
        level->GetStaticMeshFromId(level->GetDefaultStaticMeshCubeId());
    auto material_id = level->GetIdFromName("EquirectangularMaterial");
    if (!material_id)
        throw std::runtime_error(
            "No material id found for [EquirectangularMaterial].");
    MaterialInterface& material_ref = level->GetMaterialFromId(material_id);
    for (std::uint32_t i = 0; i < 6; ++i)
    {
        renderer.SetCubeMapTarget(Cubemap::GetTextureFrameFromPosition(i));
        renderer.RenderMesh(
            mesh_ref, material_ref, projection_cubemap, views_cubemap[i]);
    }
    // FIXME(anirul): Why?
    renderer.SetCubeMapTarget(Cubemap::GetTextureFrameFromPosition(0));
    renderer.RenderMesh(
        mesh_ref, material_ref, projection_cubemap, views_cubemap[0]);
    // Get the output image (cube map).
    auto maybe_output_id = level->GetIdFromName("OutputTexture");
    if (!maybe_output_id)
    {
        throw std::runtime_error("Couldn't create the cubemap?");
    }
    std::unique_ptr<TextureInterface> texture =
        level->ExtractTexture(maybe_output_id);
    Cubemap& cubemap = static_cast<Cubemap&>(*texture);
    texture_id_ = cubemap.GetId();
    cubemap.texture_id_ = 0;
    inner_size_ = cube_pair_res;
}

void Cubemap::CreateCubemapFromPointers(
    std::array<const void*, 6> cube_map,
    glm::uvec2 size,
    proto::PixelElementSize pixel_element_size,
    proto::PixelStructure pixel_structure)
{
    data_.mutable_pixel_element_size()->CopyFrom(pixel_element_size);
    data_.mutable_pixel_structure()->CopyFrom(pixel_structure);
    data_.mutable_size()->CopyFrom(json::SerializeSize(size));
    inner_size_ = size;
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
        throw std::runtime_error(fmt::format("Invalid entry {}.", i));
    }
    return texture_frame;
}

} // End namespace frame::opengl.
