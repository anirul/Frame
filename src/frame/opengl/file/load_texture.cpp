#include "frame/opengl/file/load_texture.h"

#include <algorithm>
#include <fstream>
#include <set>
#include <vector>

#include "frame/file/file_system.h"
#include "frame/file/image.h"
#include "frame/json/parse_level.h"
#include "frame/json/parse_uniform.h"
#include "frame/logger.h"
#include "frame/node_matrix.h"
#include "frame/opengl/cubemap.h"
#include "frame/opengl/material.h"
#include "frame/opengl/renderer.h"
#include "frame/opengl/static_mesh.h"
#include "frame/opengl/texture.h"
#include "frame/window_factory.h"

namespace frame::opengl::file
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
        power <<= 1;
    return power;
}

} // End namespace.

std::unique_ptr<frame::TextureInterface> LoadTextureFromFile(
    const std::filesystem::path& file,
    proto::PixelElementSize
        pixel_element_size /*= proto::PixelElementSize_BYTE()*/,
    proto::PixelStructure pixel_structure /*= proto::PixelStructure_RGB()*/)
{
    frame::file::Image image(file, pixel_element_size, pixel_structure);
    TextureParameter texture_parameter;
    texture_parameter.pixel_element_size = pixel_element_size;
    texture_parameter.pixel_structure = pixel_structure;
    texture_parameter.size = image.GetSize();
    texture_parameter.data_ptr = image.Data();
    texture_parameter.file_name = file.string();
    return std::make_unique<frame::opengl::Texture>(texture_parameter);
}

std::unique_ptr<frame::TextureInterface> LoadCubeMapTextureFromFile(
    const std::filesystem::path& file,
    proto::PixelElementSize
        pixel_element_size /*= proto::PixelElementSize_BYTE()*/,
    proto::PixelStructure pixel_structure /*= proto::PixelStructure_RGB()*/)
{
    auto& logger = Logger::GetInstance();
    auto equirectangular =
        LoadTextureFromFile(file, pixel_element_size, pixel_structure);
    if (!equirectangular)
    {
        logger->info("Could not load texture: [{}].", file.string());
        return nullptr;
    }
    auto size = json::ParseSize(equirectangular->GetData().size());
    // Seams correct when you are less than 2048 in height you get 512.
    std::uint32_t cube_single_res = PowerFloor(size.y);
    glm::uvec2 cube_pair_res = {cube_single_res, cube_single_res};
    std::map<std::string, std::string> filling_map = {
        {"<filename>", file.string()},
        {"<x>", std::to_string(cube_pair_res.x)},
        {"<y>", std::to_string(cube_pair_res.y)},
        {"<pixel_element_size>",
         PixelElementSize_Enum_Name(pixel_element_size.value())},
        {"<pixel_structure>",
         PixelStructure_Enum_Name(pixel_structure.value())}};
    // Now get it from external file.
    std::ifstream ifs(
        frame::file::FindFile("asset/json/equirectangular.json").string());
    std::string inner_file_json((std::istreambuf_iterator<char>(ifs)), {});
    auto level = json::ParseLevel(
        cube_pair_res, FillLevel(inner_file_json, filling_map));
    if (!level)
    {
        logger->info("Could not create level.");
        return nullptr;
    }
    auto out_texture_id = level->GetIdFromName("OutputTexture");
    if (out_texture_id == NullId)
    {
        logger->info("Could not get the id of \"OutputTexture\".");
        return nullptr;
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
        renderer.SetCubeMapTarget(GetTextureFrameFromPosition(i));
        renderer.RenderMesh(
            mesh_ref, material_ref, projection_cubemap, views_cubemap[i]);
    }
    // FIXME(anirul): Why?
    renderer.SetCubeMapTarget(GetTextureFrameFromPosition(0));
    renderer.RenderMesh(
        mesh_ref, material_ref, projection_cubemap, views_cubemap[0]);
    // Get the output image (cube map).
    auto maybe_output_id = level->GetIdFromName("OutputTexture");
    if (!maybe_output_id)
    {
        return nullptr;
    }
    return level->ExtractTexture(maybe_output_id);
}

std::unique_ptr<frame::TextureInterface> LoadCubeMapTextureFromFiles(
    const std::array<std::filesystem::path, 6>& files,
    proto::PixelElementSize
        pixel_element_size /*= proto::PixelElementSize_BYTE()*/,
    proto::PixelStructure pixel_structure /*= proto::PixelStructure_RGB()*/)
{
    TextureParameter texture_parameter = {pixel_element_size, pixel_structure};
    texture_parameter.map_type = TextureTypeEnum::CUBMAP;
    std::array<std::filesystem::path, 6> final_files = {};
    for (int i = 0; i < final_files.size(); ++i)
    {
        texture_parameter.array_file_names[i] = files[i].string();
        final_files[i] = frame::file::FindFile(files[i]);
    }
    std::pair<std::uint32_t, std::uint32_t> img_size;
    std::array<std::unique_ptr<frame::file::Image>, 6> images;
    std::array<void*, 6> pointers = {};
    for (int i = 0; i < pointers.size(); ++i)
    {
        images[i] = std::make_unique<frame::file::Image>(
            final_files[i], pixel_element_size, pixel_structure);
        texture_parameter.array_data_ptr[i] = images[i]->Data();
    }
    texture_parameter.size = images[0]->GetSize();
    return std::make_unique<opengl::Cubemap>(texture_parameter);
}

std::unique_ptr<TextureInterface> LoadTextureFromVec4(const glm::vec4& vec4)
{
    std::array<float, 4> ar = {vec4.x, vec4.y, vec4.z, vec4.w};
    TextureParameter texture_parameter;
    texture_parameter.pixel_element_size = json::PixelElementSize_FLOAT();
    texture_parameter.pixel_structure = json::PixelStructure_RGB_ALPHA();
    texture_parameter.size = {1, 1};
    texture_parameter.data_ptr = (void*)&ar;
    return std::make_unique<frame::opengl::Texture>(texture_parameter);
}

std::unique_ptr<TextureInterface> LoadTextureFromFloat(float f)
{
    TextureParameter texture_parameter;
    texture_parameter.pixel_element_size = json::PixelElementSize_FLOAT();
    texture_parameter.pixel_structure = json::PixelStructure_GREY();
    texture_parameter.size = {1, 1};
    texture_parameter.data_ptr = (void*)&f;
    return std::make_unique<frame::opengl::Texture>(texture_parameter);
}

} // namespace frame::opengl::file
