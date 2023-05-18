#include "frame/opengl/file/load_static_mesh.h"

#include <stdexcept>

#include "frame/file/file_system.h"
#include "frame/file/image.h"
#include "frame/file/obj.h"
#include "frame/file/ply.h"
#include "frame/logger.h"
#include "frame/opengl/buffer.h"
#include "frame/opengl/file/load_texture.h"
#include "frame/opengl/static_mesh.h"

namespace frame::opengl::file {

namespace {

template <typename T>
std::optional<EntityId> CreateBufferInLevel(
    LevelInterface& level, const std::vector<T>& vec, const std::string& desc,
    const BufferTypeEnum buffer_type = BufferTypeEnum::ARRAY_BUFFER,
    const BufferUsageEnum buffer_usage = BufferUsageEnum::STATIC_DRAW) {
  auto buffer = std::make_unique<Buffer>(buffer_type, buffer_usage);
  if (!buffer) throw std::runtime_error("No buffer create!");
  // Buffer initialization.
  buffer->Bind();
  buffer->Copy(vec.size() * sizeof(T), vec.data());
  buffer->UnBind();
  buffer->SetName(desc);
  return level.AddBuffer(std::move(buffer));
}

std::optional<std::unique_ptr<TextureInterface>> LoadTextureFromString(
    const std::string& str, const proto::PixelElementSize pixel_element_size,
    const proto::PixelStructure pixel_structure) {
  return opengl::file::LoadTextureFromFile(
      frame::file::FindFile("asset/" + str), pixel_element_size,
      pixel_structure);
}

std::optional<EntityId> LoadMaterialFromObj(
    LevelInterface& level, const frame::file::ObjMaterial& material_obj) {
  // Load textures.
  auto maybe_color = (material_obj.ambient_str.empty())
                         ? LoadTextureFromVec4(material_obj.ambient_vec4)
                         : LoadTextureFromString(material_obj.ambient_str,
                                                 proto::PixelElementSize_BYTE(),
                                                 proto::PixelStructure_RGB());
  if (!maybe_color) return std::nullopt;
  auto maybe_normal =
      (material_obj.normal_str.empty())
          ? LoadTextureFromVec4(glm::vec4(0.f, 0.f, 0.f, 1.f))
          : LoadTextureFromString(material_obj.normal_str,
                                  proto::PixelElementSize_BYTE(),
                                  proto::PixelStructure_RGB());
  if (!maybe_normal) return std::nullopt;
  auto maybe_roughness =
      (material_obj.roughness_str.empty())
          ? LoadTextureFromFloat(material_obj.roughness_val)
          : LoadTextureFromString(material_obj.roughness_str,
                                  proto::PixelElementSize_BYTE(),
                                  proto::PixelStructure_GREY());
  if (!maybe_roughness) return std::nullopt;
  auto maybe_metallic =
      (material_obj.metallic_str.empty())
          ? LoadTextureFromFloat(material_obj.metallic_val)
          : LoadTextureFromString(material_obj.metallic_str,
                                  proto::PixelElementSize_BYTE(),
                                  proto::PixelStructure_GREY());
  if (!maybe_metallic) return std::nullopt;
  // Create names for textures.
  auto color_name = fmt::format("{}.Color", material_obj.name);
  auto normal_name = fmt::format("{}.Normal", material_obj.name);
  auto roughness_name = fmt::format("{}.Roughness", material_obj.name);
  auto metallic_name = fmt::format("{}.Metallic", material_obj.name);
  // Add texture to the level.
  maybe_color.value()->SetName(color_name);
  auto maybe_color_id = level.AddTexture(std::move(maybe_color.value()));
  if (!maybe_color_id) return std::nullopt;
  maybe_normal.value()->SetName(normal_name);
  auto maybe_normal_id = level.AddTexture(std::move(maybe_normal.value()));
  if (!maybe_normal_id) return std::nullopt;
  maybe_roughness.value()->SetName(roughness_name);
  auto maybe_roughness_id =
      level.AddTexture(std::move(maybe_roughness.value()));
  if (!maybe_roughness_id) return std::nullopt;
  maybe_metallic.value()->SetName(metallic_name);
  auto maybe_metallic_id = level.AddTexture(std::move(maybe_metallic.value()));
  if (!maybe_metallic_id) return std::nullopt;
  // Create the material.
  std::unique_ptr<MaterialInterface> material =
      std::make_unique<opengl::Material>();
  // Add texture to the material.
  material->AddTextureId(maybe_color_id, color_name);
  material->AddTextureId(maybe_normal_id, normal_name);
  material->AddTextureId(maybe_roughness_id, roughness_name);
  material->AddTextureId(maybe_metallic_id, metallic_name);
  // Finally add the material to the level.
  material->SetName(material_obj.name);
  return level.AddMaterial(std::move(material));
}

std::pair<EntityId, EntityId> LoadStaticMeshFromObj(
    LevelInterface& level, const frame::file::ObjMesh& mesh_obj,
    const std::string& name, const std::vector<EntityId> material_ids,
    int counter) {
  std::vector<float> points;
  std::vector<float> normals;
  std::vector<float> textures;
  const auto& vertices = mesh_obj.GetVertices();
  // TODO(anirul): could probably short this out!
  for (const auto& vertice : vertices) {
    points.push_back(vertice.point.x);
    points.push_back(vertice.point.y);
    points.push_back(vertice.point.z);
    normals.push_back(vertice.normal.x);
    normals.push_back(vertice.normal.y);
    normals.push_back(vertice.normal.z);
    textures.push_back(vertice.tex_coord.x);
    textures.push_back(vertice.tex_coord.y);
  }
  const auto& indices = mesh_obj.GetIndices();

  // Point buffer initialization.
  auto maybe_point_buffer_id = CreateBufferInLevel(
      level, points, fmt::format("{}.{}.point", name, counter));
  if (!maybe_point_buffer_id) return {NullId, NullId};
  EntityId point_buffer_id = maybe_point_buffer_id.value();

  // Normal buffer initialization.
  auto maybe_normal_buffer_id = CreateBufferInLevel(
      level, normals, fmt::format("{}.{}.normal", name, counter));
  if (!maybe_normal_buffer_id) return {NullId, NullId};
  EntityId normal_buffer_id = maybe_normal_buffer_id.value();

  // Texture coordinates buffer initialization.
  auto maybe_tex_coord_buffer_id = CreateBufferInLevel(
      level, textures, fmt::format("{}.{}.texture", name, counter));
  if (!maybe_tex_coord_buffer_id) return {NullId, NullId};
  EntityId tex_coord_buffer_id = maybe_tex_coord_buffer_id.value();

  // Index buffer array.
  auto maybe_index_buffer_id = CreateBufferInLevel(
      level, indices, fmt::format("{}.{}.index", name, counter),
      opengl::BufferTypeEnum::ELEMENT_ARRAY_BUFFER);
  if (!maybe_index_buffer_id) return {NullId, NullId};
  EntityId index_buffer_id = maybe_index_buffer_id.value();
  StaticMeshParameter parameter = {};
  parameter.point_buffer_id = point_buffer_id;
  parameter.normal_buffer_id = normal_buffer_id;
  parameter.texture_buffer_id = tex_coord_buffer_id;
  parameter.index_buffer_id = index_buffer_id;
  auto static_mesh = std::make_unique<opengl::StaticMesh>(level, parameter);
  auto material_id = NullId;
  if (!material_ids.empty()) {
    if (material_ids.size() != 1) {
      throw std::runtime_error("should only have 1 material here.");
    }
    material_id = material_ids[0];
  }
  std::string mesh_name = fmt::format("{}.{}", name, counter);
  static_mesh->SetName(mesh_name);
  auto maybe_mesh_id = level.AddStaticMesh(std::move(static_mesh));
  if (!maybe_mesh_id) return {NullId, NullId};
  return {maybe_mesh_id, material_id};
}

EntityId LoadStaticMeshFromPly(LevelInterface& level,
                               const frame::file::Ply& ply,
                               const std::string& name) {
  EntityId result = NullId;
  std::vector<float> points;
  std::vector<float> normals;
  std::vector<float> textures;
  std::vector<float> colors;
  for (const auto& point : ply.GetVertices()) {
    points.push_back(point.x);
    points.push_back(point.y);
    points.push_back(point.z);
  }
  for (const auto& normal : ply.GetNormals()) {
    normals.push_back(normal.x);
    normals.push_back(normal.y);
    normals.push_back(normal.z);
  }
  for (const auto& color : ply.GetColors()) {
    colors.push_back(color.r);
    colors.push_back(color.g);
    colors.push_back(color.b);
  }
  for (const auto& texcoord : ply.GetTextureCoordinates()) {
    textures.push_back(texcoord.x);
    textures.push_back(texcoord.y);
  }
  const auto& indices = ply.GetIndices();

  // Point buffer initialization.
  auto maybe_point_buffer_id =
      CreateBufferInLevel(level, points, fmt::format("{}.point", name));
  if (!maybe_point_buffer_id) return NullId;
  EntityId point_buffer_id = maybe_point_buffer_id.value();

  // Color buffer initialization.
  auto maybe_color_buffer_id =
      CreateBufferInLevel(level, colors, fmt::format("{}.color", name));
  if (!maybe_color_buffer_id) return NullId;
  EntityId color_buffer_id = maybe_color_buffer_id.value();

  // Normal buffer initialization.
  auto maybe_normal_buffer_id =
      CreateBufferInLevel(level, normals, fmt::format("{}.normal", name));
  if (!maybe_normal_buffer_id) return NullId;
  EntityId normal_buffer_id = maybe_normal_buffer_id.value();

  // Texture coordinates buffer initialization.
  auto maybe_tex_coord_buffer_id =
      CreateBufferInLevel(level, textures, fmt::format("{}.texture", name));
  if (!maybe_tex_coord_buffer_id) return NullId;
  EntityId tex_coord_buffer_id = maybe_tex_coord_buffer_id.value();

  // Index buffer array.
  auto maybe_index_buffer_id =
      CreateBufferInLevel(level, indices, fmt::format("{}.index", name),
                          opengl::BufferTypeEnum::ELEMENT_ARRAY_BUFFER);
  if (!maybe_index_buffer_id) return NullId;
  EntityId index_buffer_id = maybe_index_buffer_id.value();

  std::unique_ptr<opengl::StaticMesh> static_mesh = nullptr;

  StaticMeshParameter parameter = {};
  parameter.point_buffer_id = point_buffer_id;
  parameter.index_buffer_id = index_buffer_id;

  // Add for present buffer.
  if (!normals.empty()) {
    parameter.normal_buffer_id = normal_buffer_id;
  }
  if (!textures.empty()) {
    parameter.texture_buffer_id = tex_coord_buffer_id;
  }
  if (!colors.empty()) {
    parameter.color_buffer_id = color_buffer_id;
  }

  static_mesh = std::make_unique<opengl::StaticMesh>(level, parameter);
  std::string mesh_name = fmt::format("{}", name);
  static_mesh->SetName(mesh_name);
  auto maybe_mesh_id = level.AddStaticMesh(std::move(static_mesh));
  if (!maybe_mesh_id) return NullId;
  return maybe_mesh_id;
}

std::vector<EntityId> LoadStaticMeshesFromObjFile(
    LevelInterface& level, const std::filesystem::path& file,
    const std::string& name, const std::string& material_name /* = ""*/) {
  std::vector<EntityId> entity_id_vec;
  frame::file::Obj obj(file);
  const auto& meshes = obj.GetMeshes();
  Logger& logger = Logger::GetInstance();
  std::vector<EntityId> material_ids;
  if (!material_name.empty()) {
    auto maybe_id = level.GetIdFromName(material_name);
    if (maybe_id) material_ids.push_back(maybe_id);
  }
  logger->info("Found in obj<{}> : {} meshes.", file.string(), meshes.size());
  int mesh_counter = 0;
  for (const auto& mesh : meshes) {
    auto [static_mesh_id, material_id] =
        LoadStaticMeshFromObj(level, mesh, name, material_ids, mesh_counter);
    if (!static_mesh_id) return {};
    auto func = [&level](const std::string& name) -> NodeInterface* {
      auto maybe_id = level.GetIdFromName(name);
      if (!maybe_id) {
        throw std::runtime_error(fmt::format("no id for name: {}", name));
      }
      return &level.GetSceneNodeFromId(maybe_id);
    };
    auto ptr = std::make_unique<NodeStaticMesh>(func, static_mesh_id);
    ptr->SetName(fmt::format("Node.{}.{}", name, mesh_counter));
    auto maybe_id = level.AddSceneNode(std::move(ptr));
    if (!maybe_id) return {};
    level.AddMeshMaterialId(maybe_id, material_id);
    entity_id_vec.push_back(maybe_id);
    mesh_counter++;
  }
  return entity_id_vec;
}

EntityId LoadStaticMeshFromPlyFile(LevelInterface& level,
                                   const std::filesystem::path& file,
                                   const std::string& name,
                                   const std::string& material_name /* = ""*/) {
  EntityId entity_id = NullId;
  frame::file::Ply ply(file);
  Logger& logger = Logger::GetInstance();
  EntityId material_id = NullId;
  if (!material_name.empty()) {
    auto maybe_id = level.GetIdFromName(material_name);
    if (maybe_id) material_id = maybe_id;
  }
  auto static_mesh_id = LoadStaticMeshFromPly(level, ply, name);
  if (!static_mesh_id) return NullId;
  auto func = [&level](const std::string& name) -> NodeInterface* {
    auto maybe_id = level.GetIdFromName(name);
    if (!maybe_id) {
      throw std::runtime_error(fmt::format("no id for name: {}", name));
    }
    return &level.GetSceneNodeFromId(maybe_id);
  };
  auto ptr = std::make_unique<NodeStaticMesh>(func, static_mesh_id);
  ptr->SetName(fmt::format("Node.{}", name));
  auto maybe_id = level.AddSceneNode(std::move(ptr));
  if (!maybe_id) return NullId;
  level.AddMeshMaterialId(maybe_id, material_id);
  entity_id = maybe_id;
  return entity_id;
}

}  // End namespace.

std::vector<EntityId> LoadStaticMeshesFromFile(
    LevelInterface& level, const std::filesystem::path& file,
    const std::string& name, const std::string& material_name /* = ""*/) {
  auto extension = file.extension();
  std::filesystem::path final_path = frame::file::FindFile(file);
  if (extension == ".obj")
    return LoadStaticMeshesFromObjFile(level, final_path, name, material_name);
  if (extension == ".ply")
    return {LoadStaticMeshFromPlyFile(level, final_path, name, material_name)};
  return {};
}

}  // End namespace frame::opengl::file.
