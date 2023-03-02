#include "frame/opengl/file/load_texture.h"

#include <algorithm>
#include <fstream>
#include <set>
#include <vector>

#include "frame/file/file_system.h"
#include "frame/file/image.h"
#include "frame/json/parse_level.h"
#include "frame/logger.h"
#include "frame/node_matrix.h"
#include "frame/opengl/material.h"
#include "frame/opengl/renderer.h"
#include "frame/opengl/static_mesh.h"
#include "frame/opengl/texture.h"
#include "frame/opengl/texture_cube_map.h"

namespace frame::opengl::file {

namespace {

// Get the 6 view for the cube map.
// CHECKME(anirul): Why mat4? why not mat3 or having an individual 1 in the 4th column?
const std::array<glm::mat4, 6> views_cubemap = {
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, -1.0f, 0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, -1.0f, 0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                glm::vec3(0.0f, 0.0f, 1.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
                glm::vec3(0.0f, 0.0f, -1.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
                glm::vec3(0.0f, -1.0f, 0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                glm::vec3(0.0f, -1.0f, 0.0f))
};

const std::set<std::string> byte_extention = { "jpeg", "jpg" };
const std::set<std::string> rgba_extention = { "png" };
const std::set<std::string> half_extention = { "hdr", "dds" };

const std::string proto_level_json = R"json(
				{
					"name": "Equirectangular",
					"default_texture_name": "OutputTexture",
					"programs": [
						{
							"name": "EquirectangularProgram",
							"shader": "equirectangular_cubemap",
							"input_scene_type":	{
								"value": "QUAD"
							},
							"output_texture_names": [ "OutputTexture" ]
						}
					],
					"scene_tree": {
						"default_root_name": "root",
						"default_camera_name": "camera",
						"scene_matrices": [
							{
								"name": "root",
                                "matrix": {
                                    "m11": 1,
                                    "m22": 1,
                                    "m33": 1,
                                    "m44": 1
                                }
							},
							{
								"name": "camera_boon",
								"parent": "root"
							}
						],
						"scene_static_meshes": [
							{
								"name": "Cube",
								"mesh_enum": "CUBE",
								"material_name": "EquirectangularMaterial",
								"parent": "root"
							}
						],
						"scene_cameras": [
							{
								"name": "camera",
								"parent": "camera_boon",
								"fov_degrees": "90.0",
								"near_clip": "0.1",
								"far_clip": "1000.0",
								"aspect_ratio": "1.0"
							}
						]
					},
					"textures" : [
						{
							"name": "InputTexture",
							"file_name": "<filename>",
							"cubemap": "false",
							"size" : {
								"x": "<x>",
								"y": "<y>"
							},
							"pixel_element_size": { 
								"value": "<pixel_element_size>" 
							},
							"pixel_structure": { 
								"value": "<pixel_structure>" 
							}
						},
						{
							"name": "OutputTexture",
							"cubemap": "true",
							"size": {
								"x": "<x>",
								"y": "<y>"
							},
							"pixel_element_size": { 
								"value": "<pixel_element_size>" 
							},
							"pixel_structure": { 
								"value": "<pixel_structure>" 
							}
						}
					],
					"materials": [
						{
							"name": "EquirectangularMaterial",
							"program_name": "EquirectangularProgram",
							"texture_names": [ "InputTexture" ],
							"inner_names": "Equirectangular"
						}
					]
				}
			)json";

// Taken from cpp reference.
std::size_t ReplaceAll(std::string& inout, const std::string_view what,
                       const std::string_view with) {
    std::size_t count = 0;
    for (std::string::size_type pos = 0;
         inout.npos != (pos = inout.find(what.data(), pos, what.length()));
         pos += with.length(), ++count) {
        inout.replace(pos, what.length(), with.data(), with.length());
    }
    return count;
}

std::string FillLevel(const std::string& initial, const std::map<std::string, std::string>& map) {
    std::string out = initial;
    for (auto [from, to] : map) {
        ReplaceAll(to, "\\", "/");
        ReplaceAll(out, from, to);
    }
    return out;
}

std::uint32_t PowerFloor(std::uint32_t x) {
    std::uint32_t power = 1;
    while (x >>= 1) power <<= 1;
    return power;
}
}  // namespace

std::unique_ptr<frame::TextureInterface> LoadTextureFromFile(
    const std::filesystem::path& file,
    proto::PixelElementSize pixel_element_size /*= proto::PixelElementSize_BYTE()*/,
    proto::PixelStructure pixel_structure /*= proto::PixelStructure_RGB()*/) {
    frame::file::Image image(file, pixel_element_size, pixel_structure);
    TextureParameter texture_parameter = { pixel_element_size, pixel_structure, image.GetSize(),
                                           image.Data() };
    return std::make_unique<frame::opengl::Texture>(texture_parameter);
}

std::unique_ptr<frame::TextureInterface> LoadCubeMapTextureFromFile(
    const std::filesystem::path& file,
    proto::PixelElementSize pixel_element_size /*= proto::PixelElementSize_BYTE()*/,
    proto::PixelStructure pixel_structure /*= proto::PixelStructure_RGB()*/) {
    auto& logger         = Logger::GetInstance();
    auto equirectangular = LoadTextureFromFile(file, pixel_element_size, pixel_structure);
    if (!equirectangular) {
        logger->info("Could not load texture: [{}].", file.string());
        return nullptr;
    }
    auto size = equirectangular->GetSize();
    // Seams correct when you are less than 2048 in height you get 512.
    std::uint32_t cube_single_res                         = PowerFloor(size.y) / 2;
    glm::uvec2 cube_pair_res = { cube_single_res, cube_single_res };
    std::map<std::string, std::string> filling_map        = {
               { "<filename>", file.string() },
               { "<x>", std::to_string(cube_pair_res.x) },
               { "<y>", std::to_string(cube_pair_res.y) },
               { "<pixel_element_size>", PixelElementSize_Enum_Name(pixel_element_size.value()) },
               { "<pixel_structure>", "RGB" }
    };
    auto level = frame::proto::ParseLevel(cube_pair_res, FillLevel(proto_level_json, filling_map));
    if (!level) {
        logger->info("Could not create level.");
        return nullptr;
    }
    auto maybe_id = level->GetIdFromName("OutputTexture");
    if (!maybe_id) {
        logger->info("Could not get the id of \"OutputTexture\".");
        return nullptr;
    }
    auto& out_texture_ptr = level->GetTextureFromId(maybe_id);
    Renderer renderer({ nullptr, nullptr, level.get() },
                      { 0, 0, cube_pair_res.x, cube_pair_res.y });
    StaticMeshInterface& mesh_ptr = level->GetStaticMeshFromId(level->GetDefaultStaticMeshQuadId());
    auto material_id = level->GetIdFromName("EquirectangularMaterial");
    if (material_id == NullId) {
        throw std::runtime_error("No material id found for [EquirectangularMaterial].");
    }
    MaterialInterface& material_ptr = level->GetMaterialFromId(material_id);
    for (std::uint32_t i = 0; i < 6; ++i) {
        renderer.SetCubeMapTarget(GetTextureFrameFromPosition(i));
        renderer.RenderMesh(mesh_ptr, material_ptr, views_cubemap[i]);
    }
    auto maybe_output_id = level->GetIdFromName("OutputTexture");
    if (!maybe_output_id) return nullptr;
    return level->ExtractTexture(maybe_output_id);
}

std::unique_ptr<frame::TextureInterface> LoadCubeMapTextureFromFiles(
    const std::array<std::filesystem::path, 6>& files,
    proto::PixelElementSize pixel_element_size /*= proto::PixelElementSize_BYTE()*/,
    proto::PixelStructure pixel_structure /*= proto::PixelStructure_RGB()*/) {
    std::array<std::filesystem::path, 6> final_files = {};
    for (int i = 0; i < final_files.size(); ++i) {
        final_files[i] = frame::file::FindFile(files[i]);
    }
    std::pair<std::uint32_t, std::uint32_t> img_size;
    std::array<std::unique_ptr<frame::file::Image>, 6> images;
    std::array<void*, 6> pointers      = {};
    TextureParameter texture_parameter = { pixel_element_size, pixel_structure };
    texture_parameter.map_type         = TextureTypeEnum::CUBMAP;
    for (int i = 0; i < pointers.size(); ++i) {
        images[i] = std::make_unique<frame::file::Image>(final_files[i], pixel_element_size,
                                                         pixel_structure);
        texture_parameter.array_data_ptr[i] = images[i]->Data();
    }
    texture_parameter.size = images[0]->GetSize();
    return std::make_unique<opengl::TextureCubeMap>(texture_parameter);
}

std::unique_ptr<TextureInterface> LoadTextureFromVec4(const glm::vec4& vec4) {
    std::array<float, 4> ar            = { vec4.x, vec4.y, vec4.z, vec4.w };
    TextureParameter texture_parameter = {
        proto::PixelElementSize_FLOAT(), proto::PixelStructure_RGB_ALPHA(), { 1, 1 }, (void*)&ar
    };
    return std::make_unique<frame::opengl::Texture>(texture_parameter);
}

std::unique_ptr<TextureInterface> LoadTextureFromFloat(float f) {
    TextureParameter texture_parameter = { frame::proto::PixelElementSize_FLOAT(),
                                           frame::proto::PixelStructure_GREY(),
                                           { 1, 1 },
                                           (void*)&f };
    return std::make_unique<frame::opengl::Texture>(texture_parameter);
}

}  // namespace frame::opengl::file
