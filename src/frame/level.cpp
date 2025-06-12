#include "frame/level.h"

#include <algorithm>
#include <format>
#include <numeric>

#include "frame/device_interface.h"
#include "frame/node_camera.h"
#include "frame/node_light.h"
#include "frame/node_matrix.h"
#include "frame/node_static_mesh.h"

namespace frame
{

Level::~Level()
{
    // This has to be deleted first (it has reference to buffers).
    id_static_mesh_map_.clear();
}

std::vector<std::pair<EntityId, EntityId>> Level::GetStaticMeshMaterialIds(
    proto::NodeStaticMesh::RenderTimeEnum render_time_enum) const
{
    switch (render_time_enum)
    {
    case proto::NodeStaticMesh::PRE_RENDER_TIME:
        return mesh_material_pre_render_ids_;
    case proto::NodeStaticMesh::SCENE_RENDER_TIME:
        return mesh_material_scene_render_ids_;
    case proto::NodeStaticMesh::POST_PROCESS_TIME:
        return mesh_material_post_proccess_ids_;
    case proto::NodeStaticMesh::SKYBOX_RENDER_TIME:
        return mesh_material_skybox_ids_;
    default:
        throw std::runtime_error("Unknown render time?");
    }
}

void Level::AddMeshMaterialId(
    EntityId node_id,
    EntityId material_id,
    proto::NodeStaticMesh::RenderTimeEnum render_time_enum)
{
    switch (render_time_enum)
    {
    case proto::NodeStaticMesh::PRE_RENDER_TIME:
        mesh_material_pre_render_ids_.push_back({node_id, material_id});
        break;
    case proto::NodeStaticMesh::SCENE_RENDER_TIME:
        mesh_material_scene_render_ids_.push_back({node_id, material_id});
        break;
    case proto::NodeStaticMesh::POST_PROCESS_TIME:
        mesh_material_post_proccess_ids_.push_back({node_id, material_id});
        break;
    case proto::NodeStaticMesh::SKYBOX_RENDER_TIME:
        mesh_material_skybox_ids_.push_back({node_id, material_id});
        break;
    default:
        throw std::runtime_error("Unknown render time?");
    }
}

EntityId Level::GetDefaultStaticMeshQuadId() const
{
    if (quad_id_)
    {
        return quad_id_;
    }
    return NullId;
}

EntityId Level::GetDefaultStaticMeshCubeId() const
{
    if (cube_id_)
    {
        return cube_id_;
    }
    return NullId;
}

EntityId Level::GetIdFromName(const std::string& name) const
{
    if (name.empty())
    {
        logger_->warn("name is empty.");
        return NullId;
    }
    try
    {
        return name_id_map_.at(name);
    }
    catch (std::out_of_range& ex)
    {
        logger_->warn(ex.what());
        return NullId;
    }
}

std::string Level::GetNameFromId(EntityId id) const
{
    if (id == NullId)
    {
        throw std::runtime_error("Invalid id?");
    }
    if (!id_name_map_.count(id))
    {
        throw std::runtime_error(std::format("No name for id[{}]", id));
    }
    return id_name_map_.at(id);
}

EntityId Level::AddSceneNode(std::unique_ptr<NodeInterface>&& scene_node)
{
    EntityId id = GetSceneNodeNewId();
    std::string name = GetNameFromNodeInterface(*scene_node);
    // CHECKME(anirul): maybe this should return std::nullopt.
    if (string_set_.count(name))
    {
        throw std::runtime_error("Name: " + name + " is already in!");
    }
    string_set_.insert(name);
    id_scene_node_map_.insert({id, std::move(scene_node)});
    id_name_map_.insert({id, name});
    name_id_map_.insert({name, id});
    id_enum_map_.insert({id, EntityTypeEnum::NODE});
    return id;
}

EntityId Level::AddTexture(std::unique_ptr<TextureInterface>&& texture)
{
    EntityId id = GetTextureNewId();
    std::string name = texture->GetName();
    if (string_set_.count(name))
    {
        throw std::runtime_error("Name: " + name + " is already in!");
    }
    string_set_.insert(name);
    id_texture_map_.insert({id, std::move(texture)});
    id_name_map_.insert({id, name});
    name_id_map_.insert({name, id});
    id_enum_map_.insert({id, EntityTypeEnum::TEXTURE});
    return id;
}

EntityId Level::AddProgram(std::unique_ptr<ProgramInterface>&& program)
{
    EntityId id = GetProgramNewId();
    std::string name = program->GetName();
    if (string_set_.count(name))
    {
        throw std::runtime_error("Name: " + name + " is already in!");
    }
    id_program_map_.insert({id, std::move(program)});
    id_name_map_.insert({id, name});
    name_id_map_.insert({name, id});
    id_enum_map_.insert({id, EntityTypeEnum::PROGRAM});
    return id;
}

EntityId Level::AddMaterial(std::unique_ptr<MaterialInterface>&& material)
{
    EntityId id = GetMaterialNewId();
    std::string name = material->GetName();
    // CHECKME(anirul): maybe this should return std::nullopt.
    if (string_set_.count(name))
    {
        throw std::runtime_error("Name: " + name + " is already in!");
    }
    id_material_map_.insert({id, std::move(material)});
    id_name_map_.insert({id, name});
    name_id_map_.insert({name, id});
    id_enum_map_.insert({id, EntityTypeEnum::MATERIAL});
    return id;
}

EntityId Level::AddBuffer(std::unique_ptr<BufferInterface>&& buffer)
{
    EntityId id = GetBufferNewId();
    std::string name = buffer->GetName();
    // CHECKME(anirul): maybe this should return std::nullopt.
    if (string_set_.count(name))
    {
        throw std::runtime_error("Name: " + name + " is already in!");
    }
    id_buffer_map_.insert({id, std::move(buffer)});
    id_name_map_.insert({id, name});
    name_id_map_.insert({name, id});
    id_enum_map_.insert({id, EntityTypeEnum::BUFFER});
    return id;
}

EntityId Level::AddLight(std::unique_ptr<LightInterface>&& light)
{
    EntityId id = GetLightNewId();
    std::string name = light->GetName();
    // CHECKME(anirul): maybe this should return std::nullopt.
    if (string_set_.count(name))
    {
        throw std::runtime_error("Name: " + name + " is already in!");
    }
    id_light_map_.insert({id, std::move(light)});
    id_name_map_.insert({id, name});
    name_id_map_.insert({name, id});
    id_enum_map_.insert({id, EntityTypeEnum::LIGHT});
    return id;
}

void Level::RemoveBuffer(EntityId buffer_id)
{
    if (!id_buffer_map_.count(buffer_id))
    {
        throw std::runtime_error(
            fmt::format("No buffer with id #{}.", buffer_id));
    }
    std::string name = id_name_map_.at(buffer_id);
    id_buffer_map_.erase(buffer_id);
    id_name_map_.erase(buffer_id);
    name_id_map_.erase(name);
    id_enum_map_.erase(buffer_id);
}

EntityId Level::AddStaticMesh(
    std::unique_ptr<StaticMeshInterface>&& static_mesh)
{
    EntityId id = GetStaticMeshNewId();
    std::string name = static_mesh->GetName();
    // CHECKME(anirul): maybe this should return std::nullopt.
    if (string_set_.count(name))
    {
        throw std::runtime_error("Name: " + name + " is already in!");
    }
    string_set_.insert(name);
    id_static_mesh_map_.insert({id, std::move(static_mesh)});
    id_name_map_.insert({id, name});
    name_id_map_.insert({name, id});
    id_enum_map_.insert({id, EntityTypeEnum::STATIC_MESH});
    return id;
}

std::vector<frame::EntityId> Level::GetChildList(EntityId id) const
{
    std::vector<EntityId> list;
    try
    {
        const auto& node = id_scene_node_map_.at(id);
        // Check who has node as a parent.
        for (const auto& id_node : id_scene_node_map_)
        {
            // In case this is node then add it to the list.
            if (id_node.second->GetParentName() ==
                GetNameFromNodeInterface(*node))
            {
                list.push_back(id_node.first);
            }
        }
    }
    catch (std::out_of_range& ex)
    {
        logger_->warn(ex.what());
    }
    return list;
}

EntityId Level::GetParentId(EntityId id) const
{
    try
    {
        std::string name = id_scene_node_map_.at(id)->GetParentName();
        auto maybe_id = GetIdFromName(name);
        return maybe_id;
    }
    catch (std::out_of_range& ex)
    {
        logger_->warn(ex.what());
        return NullId;
    }
}

std::vector<frame::EntityId> Level::GetTextures() const
{
    std::vector<EntityId> list;
    for (const auto& [texture_id, _] : id_texture_map_)
    {
        list.push_back(texture_id);
    }
    return list;
}

std::vector<frame::EntityId> Level::GetLights() const
{
    std::vector<EntityId> list;
    for (const auto& [light_id, _] : id_light_map_)
    {
        list.push_back(light_id);
    }
    return list;
}

std::vector<frame::EntityId> Level::GetPrograms() const
{
    std::vector<EntityId> list;
    for (const auto& [program_id, _] : id_program_map_)
    {
        list.push_back(program_id);
    }
    return list;
}

std::vector<frame::EntityId> Level::GetMaterials() const
{
    std::vector<EntityId> list;
    for (const auto& [material_id, _] : id_material_map_)
    {
        list.push_back(material_id);
    }
    return list;
}

std::vector<frame::EntityId> Level::GetSceneNodes() const
{
    std::vector<EntityId> list;
    for (const auto& [node_id, _] : id_scene_node_map_)
    {
        list.push_back(node_id);
    }
    return list;
}

std::unique_ptr<frame::TextureInterface> Level::ExtractTexture(EntityId id)
{
    auto node_texture = id_texture_map_.extract(id);
    auto node_name = id_name_map_.extract(id);
    auto node_id = name_id_map_.extract(node_name.mapped());
    auto node_enum = id_enum_map_.extract(id);
    return std::move(node_texture.mapped());
}

frame::CameraInterface& Level::GetDefaultCamera()
{
    auto camera_id = GetDefaultCameraId();
    assert(camera_id);
    auto& node_interface = GetSceneNodeFromId(camera_id);
    auto& node_camera = dynamic_cast<NodeCamera&>(node_interface);
    return node_camera.GetCamera();
}

void Level::ReplaceTexture(
    std::vector<std::uint8_t>&& vector,
    glm::uvec2 size,
    std::uint8_t bytes_per_pixel,
    EntityId id)
{
    if (!id_texture_map_.count(id))
    {
        throw std::runtime_error(
            "trying to replace {} but no texture there yet?");
    }
    auto& texture = id_texture_map_.at(id);
    if (!texture)
    {
        throw std::runtime_error(
            fmt::format("Invalid texture tried to be updated {}.", id));
    }
    texture->Update(std::move(vector), size, bytes_per_pixel);
}

void Level::ReplaceMesh(
    std::unique_ptr<StaticMeshInterface>&& mesh, EntityId id)
{
    if (!id_static_mesh_map_.count(id))
    {
        throw std::runtime_error(
            fmt::format(
                "trying to replace {} by {} but no mesh there yet?",
                mesh->GetName(),
                id));
    }
    id_static_mesh_map_.erase(id);
    id_static_mesh_map_.emplace(id, std::move(mesh));
}

std::string Level::GetNameFromNodeInterface(const NodeInterface& node) const
{
    switch (node.GetNodeType())
    {
    case NodeTypeEnum::NODE_MATRIX:
        return dynamic_cast<const NodeMatrix&>(node).GetData().name();
    case NodeTypeEnum::NODE_CAMERA:
        return dynamic_cast<const NodeCamera&>(node).GetData().name();
    case NodeTypeEnum::NODE_LIGHT:
        return dynamic_cast<const NodeLight&>(node).GetData().name();
    case NodeTypeEnum::NODE_STATIC_MESH:
        return dynamic_cast<const NodeStaticMesh&>(node).GetData().name();
    default:
        throw std::runtime_error(
            std::format(
                "Unknown node type [{}]?",
                static_cast<int>(node.GetNodeType())));
    }
}

} // End namespace frame.
