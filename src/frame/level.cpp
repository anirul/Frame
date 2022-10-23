#include "frame/level.h"

#include <algorithm>
#include <numeric>

#include "frame/device_interface.h"
#include "frame/node_camera.h"

namespace frame {

Level::~Level() {
    // This has to be deleted first (it has reference to buffers).
    id_static_mesh_map_.clear();
}

EntityId Level::GetDefaultStaticMeshQuadId() const {
    if (quad_id_) {
        return quad_id_;
    }
    return NullId;
}

EntityId Level::GetDefaultStaticMeshCubeId() const {
    if (cube_id_) {
        return cube_id_;
    }
    return NullId;
}

EntityId Level::GetIdFromName(const std::string& name) const {
    if (name.empty()) {
        logger_->warn("name is empty.");
        return NullId;
    }
    try {
        return name_id_map_.at(name);
    } catch (std::out_of_range& ex) {
        logger_->warn(ex.what());
        return NullId;
    }
}

std::optional<std::string> Level::GetNameFromId(EntityId id) const {
    try {
        return id_name_map_.at(id);
    } catch (std::out_of_range& ex) {
        logger_->warn(ex.what());
        return std::nullopt;
    }
}

EntityId Level::AddSceneNode(std::unique_ptr<NodeInterface>&& scene_node) {
    EntityId id      = GetSceneNodeNewId();
    std::string name = scene_node->GetName();
    // CHECKME(anirul): maybe this should return std::nullopt.
    if (string_set_.count(name)) throw std::runtime_error("Name: " + name + " is already in!");
    string_set_.insert(name);
    id_scene_node_map_.insert({ id, std::move(scene_node) });
    id_name_map_.insert({ id, name });
    name_id_map_.insert({ name, id });
    id_enum_map_.insert({ id, EntityTypeEnum::NODE });
    return id;
}

EntityId Level::AddTexture(std::unique_ptr<TextureInterface>&& texture) {
    EntityId id      = GetTextureNewId();
    std::string name = texture->GetName();
    // CHECKME(anirul): maybe this should return std::nullopt.
    if (string_set_.count(name)) throw std::runtime_error("Name: " + name + " is already in!");
    string_set_.insert(name);
    id_texture_map_.insert({ id, std::move(texture) });
    id_name_map_.insert({ id, name });
    name_id_map_.insert({ name, id });
    id_enum_map_.insert({ id, EntityTypeEnum::TEXTURE });
    return id;
}

EntityId Level::AddProgram(std::unique_ptr<ProgramInterface>&& program) {
    EntityId id      = GetProgramNewId();
    std::string name = program->GetName();
    // CHECKME(anirul): maybe this should return std::nullopt.
    if (string_set_.count(name)) throw std::runtime_error("Name: " + name + " is already in!");
    id_program_map_.insert({ id, std::move(program) });
    id_name_map_.insert({ id, name });
    name_id_map_.insert({ name, id });
    id_enum_map_.insert({ id, EntityTypeEnum::PROGRAM });
    return id;
}

EntityId Level::AddMaterial(std::unique_ptr<MaterialInterface>&& material) {
    EntityId id      = GetMaterialNewId();
    std::string name = material->GetName();
    // CHECKME(anirul): maybe this should return std::nullopt.
    if (string_set_.count(name)) throw std::runtime_error("Name: " + name + " is already in!");
    id_material_map_.insert({ id, std::move(material) });
    id_name_map_.insert({ id, name });
    name_id_map_.insert({ name, id });
    id_enum_map_.insert({ id, EntityTypeEnum::MATERIAL });
    return id;
}

EntityId Level::AddBuffer(std::unique_ptr<BufferInterface>&& buffer) {
    EntityId id      = GetBufferNewId();
    std::string name = buffer->GetName();
    // CHECKME(anirul): maybe this should return std::nullopt.
    if (string_set_.count(name)) throw std::runtime_error("Name: " + name + " is already in!");
    id_buffer_map_.insert({ id, std::move(buffer) });
    id_name_map_.insert({ id, name });
    name_id_map_.insert({ name, id });
    id_enum_map_.insert({ id, EntityTypeEnum::BUFFER });
    return id;
}

void Level::RemoveBuffer(EntityId buffer_id) {
    if (!id_buffer_map_.count(buffer_id)) {
        throw std::runtime_error(fmt::format("No buffer with id #{}.", buffer_id));
    }
    std::string name = id_name_map_.at(buffer_id);
    id_buffer_map_.erase(buffer_id);
    id_name_map_.erase(buffer_id);
    name_id_map_.erase(name);
    id_enum_map_.erase(buffer_id);
}

EntityId Level::AddStaticMesh(std::unique_ptr<StaticMeshInterface>&& static_mesh) {
    EntityId id      = GetStaticMeshNewId();
    std::string name = static_mesh->GetName();
    // CHECKME(anirul): maybe this should return std::nullopt.
    if (string_set_.count(name)) throw std::runtime_error("Name: " + name + " is already in!");
    string_set_.insert(name);
    id_static_mesh_map_.insert({ id, std::move(static_mesh) });
    id_name_map_.insert({ id, name });
    name_id_map_.insert({ name, id });
    id_enum_map_.insert({ id, EntityTypeEnum::STATIC_MESH });
    return id;
}

EntityId Level::AddBufferStream(StreamInterface<float>* stream) {
    EntityId id      = GetStreamNewId();
    std::string name = stream->GetName();
    // CHECKME(anirul): maybe this should return std::nullopt.
    if (string_set_.count(name)) throw std::runtime_error("Name: " + name + " is already in!");
    string_set_.insert(name);
    id_buffer_stream_map_.insert({ id, std::move(stream) });
    id_name_map_.insert({ id, name });
    name_id_map_.insert({ name, id });
    id_enum_map_.insert({ id, EntityTypeEnum::STREAM });
    return id;
}

EntityId Level::AddUniformFloatStream(StreamInterface<float>* stream) {
    EntityId id      = GetStreamNewId();
    std::string name = stream->GetName();
    // CHECKME(anirul): maybe this should return std::nullopt.
    if (string_set_.count(name)) throw std::runtime_error("Name: " + name + " is already in!");
    string_set_.insert(name);
    id_uniform_float_stream_map_.insert({ id, std::move(stream) });
    id_name_map_.insert({ id, name });
    name_id_map_.insert({ name, id });
    id_enum_map_.insert({ id, EntityTypeEnum::STREAM });
    return id;
}

EntityId Level::AddUniformIntStream(StreamInterface<std::int32_t>* stream) {
    EntityId id      = GetStreamNewId();
    std::string name = stream->GetName();
    // CHECKME(anirul): maybe this should return std::nullopt.
    if (string_set_.count(name)) throw std::runtime_error("Name: " + name + " is already in!");
    string_set_.insert(name);
    id_uniform_int_stream_map_.insert({ id, std::move(stream) });
    id_name_map_.insert({ id, name });
    name_id_map_.insert({ name, id });
    id_enum_map_.insert({ id, EntityTypeEnum::STREAM });
    return id;
}

EntityId Level::AddTextureStream(StreamInterface<std::uint8_t>* stream) {
    EntityId id      = GetStreamNewId();
    std::string name = stream->GetName();
    // CHECKME(anirul): maybe this should return std::nullopt.
    if (string_set_.count(name)) throw std::runtime_error("Name: " + name + " is already in!");
    string_set_.insert(name);
    id_texture_stream_map_.insert({ id, std::move(stream) });
    id_name_map_.insert({ id, name });
    name_id_map_.insert({ name, id });
    id_enum_map_.insert({ id, EntityTypeEnum::STREAM });
    return id;
}

std::optional<std::vector<frame::EntityId>> Level::GetChildList(EntityId id) const {
    std::vector<EntityId> list;
    try {
        const auto& node = id_scene_node_map_.at(id);
        // Check who has node as a parent.
        for (const auto& id_node : id_scene_node_map_) {
            // In case this is node then add it to the list.
            if (id_node.second->GetParentName() == node->GetName()) {
                list.push_back(id_node.first);
            }
        }
    } catch (std::out_of_range& ex) {
        logger_->warn(ex.what());
        return std::nullopt;
    }
    return list;
}

EntityId Level::GetParentId(EntityId id) const {
    try {
        std::string name = id_scene_node_map_.at(id)->GetParentName();
        auto maybe_id    = GetIdFromName(name);
        return maybe_id;
    } catch (std::out_of_range& ex) {
        logger_->warn(ex.what());
        return NullId;
    }
}

std::unique_ptr<frame::TextureInterface> Level::ExtractTexture(EntityId id) {
    auto ptr = GetTextureFromId(id);
    if (!ptr) return nullptr;
    auto node_texture = id_texture_map_.extract(id);
    auto node_name    = id_name_map_.extract(id);
    auto node_id      = name_id_map_.extract(node_name.mapped());
    auto node_enum    = id_enum_map_.extract(id);
    return std::move(node_texture.mapped());
}

frame::Camera* Level::GetDefaultCamera() {
    auto maybe_camera_id = GetDefaultCameraId();
    if (!maybe_camera_id) {
        logger_->info("Could not get the camera id.");
        return nullptr;
    }
    auto camera_id      = maybe_camera_id;
    auto node_interface = GetSceneNodeFromId(camera_id);
    if (!node_interface) {
        logger_->info("Could not get node interface.");
        return nullptr;
    }
    auto node_camera = dynamic_cast<NodeCamera*>(node_interface);
    if (!node_camera) {
        logger_->info("Could not get node camera ptr.");
        return nullptr;
    }
    return node_camera->GetCamera();
}

void Level::AddStreamTextureCorrespondence(EntityId stream_id, EntityId texture_id) {
    if (!id_texture_stream_map_.count(stream_id))
        throw std::runtime_error(fmt::format("No {} stream found.", stream_id));
    if (!id_texture_map_.count(texture_id))
        throw std::runtime_error(fmt::format("No {} texture found.", texture_id));
    stream_texture_ids_.push_back({ stream_id, texture_id });
}

void Level::AddStreamBufferCorrespondence(EntityId stream_id, EntityId buffer_id) {
    if (!id_buffer_stream_map_.count(stream_id))
        throw std::runtime_error(fmt::format("No {} stream found.", stream_id));
    for (const auto& p : id_static_mesh_map_) {
        const auto* mesh = p.second.get();
        if (mesh->GetPointBufferId() == buffer_id) {
            stream_static_mesh_ids_.push_back({ stream_id, p.first });
            return;
        }
    }
    throw std::runtime_error(
        fmt::format("Error didn't find any mesh with point buffer id {}", buffer_id));
}

std::vector<frame::EntityId> Level::GetUniformFloatStreamIds() const {
    std::vector<EntityId> list;
    for (const auto& [id, _] : id_uniform_float_stream_map_) {
        list.push_back(id);
    }
    return list;
}

std::vector<frame::EntityId> Level::GetUniformIntStreamIds() const {
    std::vector<EntityId> list;
    for (const auto& [id, _] : id_uniform_int_stream_map_) {
        list.push_back(id);
    }
    return list;
}

void Level::Update(DeviceInterface* device, ProgramInterface* program, double dt /*= 0.0*/) {
    for (const auto [stream_id, texture_id] : stream_texture_ids_) {
        auto* stream = GetTextureStreamFromId(stream_id);
        auto vector  = stream->ExtractVector();
        if (vector.empty()) continue;
        ReplaceTexture(std::move(vector), stream->GetSize(), stream->GetBytesPerPixel(),
                       texture_id);
    }
    for (const auto [stream_id, static_mesh_id] : stream_static_mesh_ids_) {
        auto* stream = GetBufferStreamFromId(stream_id);
        auto vector  = stream->ExtractVector();
        // In case nothing is set just skip it!
        if (vector.empty()) continue;
        // This is here that I sheet the system by inputing 4 in the number of float.
        auto mesh = device->CreateStaticMesh(std::move(vector), 4);
        mesh->SetName(id_name_map_.at(static_mesh_id));
        ReplaceMesh(std::move(mesh), static_mesh_id);
    }
    // We don't need to update the uniform as they are updated from the uniform wrapper class.
}

void Level::ReplaceTexture(std::vector<std::uint8_t>&& vector,
                           std::pair<std::uint32_t, std::uint32_t> size,
                           std::uint8_t bytes_per_pixel, EntityId id) {
    if (!id_texture_map_.count(id))
        throw std::runtime_error("trying to replace {} but no texture there yet?");
    auto& texture = id_texture_map_.at(id);
    if (!texture)
        throw std::runtime_error(fmt::format("Invalid texture tried to be updated {}.", id));
    texture->Update(std::move(vector), size, bytes_per_pixel);
}

void Level::ReplaceMesh(std::unique_ptr<StaticMeshInterface>&& mesh, EntityId id) {
    if (!id_static_mesh_map_.count(id))
        throw std::runtime_error("trying to replace {} but no mesh there yet?");
    id_static_mesh_map_.erase(id);
    id_static_mesh_map_.emplace(id, std::move(mesh));
}

}  // End namespace frame.
