#include "frame/level.h"

#include <algorithm>
#include <format>
#include <numeric>

#include "frame/device_interface.h"
#include "frame/json/parse_uniform.h"
#include "frame/node_camera.h"
#include "frame/node_light.h"
#include "frame/node_matrix.h"
#include "frame/node_static_mesh.h"
#include "frame/opengl/light.h"

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
    bool is_light_node = dynamic_cast<NodeLight*>(scene_node.get()) != nullptr;
    if (!is_light_node)
    {
        string_set_.insert(name);
    }
    id_scene_node_map_.insert({id, std::move(scene_node)});
    id_name_map_.insert({id, name});
    name_id_map_.insert({name, id});
    id_enum_map_.insert({id, EntityTypeEnum::NODE});
    // Now check if this is a light and add it to the light map.
    NodeInterface* node = id_scene_node_map_.at(id).get();
    if (auto* node_light = dynamic_cast<NodeLight*>(node))
    {
        const auto& data = node_light->GetData();
        std::unique_ptr<LightInterface> light = nullptr;
        switch (data.light_type())
        {
        case proto::NodeLight::POINT_LIGHT: {
            glm::vec3 pos = json::ParseUniform(data.position());
            light = std::make_unique<opengl::LightPoint>(
                pos,
                json::ParseUniform(data.color()),
                static_cast<ShadowTypeEnum>(data.shadow_type()));
            break;
        }
        case proto::NodeLight::DIRECTIONAL_LIGHT: {
            glm::vec3 dir = json::ParseUniform(data.direction());
            glm::mat4 model = node_light->GetLocalModel(0.0);
            glm::vec3 world_dir = glm::mat3(model) * dir;
            world_dir = glm::normalize(world_dir);
            light = std::make_unique<opengl::LightDirectional>(
                world_dir,
                json::ParseUniform(data.color()),
                static_cast<ShadowTypeEnum>(data.shadow_type()));
            break;
        }
        default:
            // Other light types not handled yet.
            break;
        }
        if (light)
        {
            light->SetName(data.name());
            EntityId light_id = this->AddLight(std::move(light));
            node_light_to_light_map_.insert({id, light_id});
        }
    }
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

void Level::RemoveProgram(EntityId program_id)
{
    if (!id_program_map_.count(program_id))
    {
        throw std::runtime_error(
            std::format("No program with id #{}.", program_id));
    }
    for (const auto& [material_id, material_ptr] : id_material_map_)
    {
        try
        {
            if (material_ptr->GetProgramId(this) == program_id)
            {
                throw std::runtime_error(
                    std::format(
                        "Program '{}' is still used by material '{}'",
                        id_name_map_.at(program_id),
                        id_name_map_.at(material_id)));
            }
        }
        catch (...)
        {
        }
    }
    std::string name = id_name_map_.at(program_id);
    id_program_map_.erase(program_id);
    id_name_map_.erase(program_id);
    name_id_map_.erase(name);
    id_enum_map_.erase(program_id);
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
    string_set_.insert(name);
    id_light_map_.insert({id, std::move(light)});
    id_name_map_.insert({id, name});
    name_id_map_.insert({name, id});
    id_enum_map_.insert({id, EntityTypeEnum::LIGHT});
    return id;
}

void Level::RemoveSceneNode(EntityId node_id)
{
    if (!id_scene_node_map_.count(node_id))
    {
        throw std::runtime_error(
            std::format("No scene node with id #{}.", node_id));
    }
    std::string name = id_name_map_.at(node_id);
    id_scene_node_map_.erase(node_id);
    id_name_map_.erase(node_id);
    name_id_map_.erase(name);
    id_enum_map_.erase(node_id);
}

void Level::RemoveBuffer(EntityId buffer_id)
{
    if (!id_buffer_map_.count(buffer_id))
    {
        throw std::runtime_error(
            std::format("No buffer with id #{}.", buffer_id));
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

void Level::UpdateLights(double dt)
{
    for (const auto& [node_id, light_id] : node_light_to_light_map_)
    {
        auto* node_interface = id_scene_node_map_.at(node_id).get();
        auto* node_light = dynamic_cast<NodeLight*>(node_interface);
        if (!node_light)
        {
            continue;
        }
        const auto& data = node_light->GetData();
        glm::mat4 model = node_light->GetLocalModel(dt);
        auto& light = *id_light_map_.at(light_id);
        switch (data.light_type())
        {
        case proto::NodeLight::POINT_LIGHT: {
            glm::vec3 pos = json::ParseUniform(data.position());
            glm::vec3 world_pos = glm::vec3(model * glm::vec4(pos, 1.0f));
            light.SetVector(world_pos);
            break;
        }
        case proto::NodeLight::DIRECTIONAL_LIGHT: {
            glm::vec3 dir = json::ParseUniform(data.direction());
            glm::vec3 world_dir = glm::mat3(model) * dir;
            world_dir = glm::normalize(world_dir);
            light.SetVector(world_dir);
            break;
        }
        default:
            break;
        }
    }
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
            std::format("Invalid texture tried to be updated {}.", id));
    }
    texture->Update(std::move(vector), size, bytes_per_pixel);
}

void Level::ReplaceMesh(
    std::unique_ptr<StaticMeshInterface>&& mesh, EntityId id)
{
    if (!id_static_mesh_map_.count(id))
    {
        throw std::runtime_error(
            std::format(
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

CameraInterface& Level::GetCameraFromId(EntityId id) const
{
    auto& node = GetSceneNodeFromId(id);
    auto& camera_node = dynamic_cast<NodeCamera&>(node);
    return camera_node.GetCamera();
}

} // End namespace frame.
