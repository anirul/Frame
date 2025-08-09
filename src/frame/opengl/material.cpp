#include "material.h"

#include <cassert>
#include <sstream>

namespace frame::opengl
{

bool Material::AddTextureId(EntityId id, const std::string& name)
{
    RemoveTextureId(id);
    return id_name_map_.insert({id, name}).second;
}

bool Material::HasTextureId(EntityId id) const
{
    return static_cast<bool>(id_name_map_.count(id));
}

std::string Material::GetInnerName(EntityId id) const
{
    return id_name_map_.at(id);
}

bool Material::RemoveTextureId(EntityId id)
{
    if (!HasTextureId(id))
        return false;
    auto it = id_name_map_.find(id);
    id_name_map_.erase(it);
    return true;
}

std::pair<std::string, int> Material::EnableTextureId(EntityId id) const
{
    // Check it exist.
    if (!HasTextureId(id))
        throw std::runtime_error("No texture id: " + std::to_string(id));
    // Check it is not already enabled.
    for (const auto& i : id_array_)
    {
        if (i == id)
        {
            throw std::runtime_error("Already in?");
        }
    }
    // Assign it.
    for (int i = 0; i < id_array_.size(); ++i)
    {
        if (id_array_[i] == 0)
        {
            id_array_[i] = id;
            return {id_name_map_.at(id), i};
        }
    }
    // No free slots.
    throw std::runtime_error("No free slots!");
}

void Material::DisableTextureId(EntityId id) const
{
    // Check if exist.
    if (!HasTextureId(id))
        throw std::runtime_error("No texture id: " + std::to_string(id));
    // Disable it.
    for (auto& i : id_array_)
    {
        if (i == id)
        {
            i = 0;
            return;
        }
    }
    // Error not found in the enable array.
    throw std::runtime_error(
        "Texture id: " + std::to_string(id) + " was not bind to any slots.");
}

void Material::DisableAll() const
{
    for (int i = 0; i < 32; ++i)
    {
        if (id_array_[i])
        {
            DisableTextureId(id_array_[i]);
        }
    }
}

std::vector<EntityId> Material::GetTextureIds() const
{
    std::vector<EntityId> vec;
    for (const auto& p : id_name_map_)
    {
        vec.push_back(p.first);
    }
    return vec;
}

frame::EntityId Material::GetProgramId(
    const LevelInterface* level /*= nullptr*/) const
{
    if (program_id_)
    {
        return program_id_;
    }
    auto maybe_id = level->GetIdFromName(program_name_);
    if (maybe_id)
    {
        program_id_ = maybe_id;
        return program_id_;
    }
    throw std::runtime_error("No valid program!");
}

void Material::SetProgramId(EntityId id)
{
    if (!id)
    {
        throw std::runtime_error("Not a valid program id.");
    }
    // TODO(anirul): Check that the program has a valid uniform!
    program_id_ = id;
}

std::string Material::GetInnerBufferName(EntityId id) const
{
    return id_buffer_name_map_.at(id);
}

bool Material::AddBufferId(EntityId id, const std::string& name)
{
    RemoveBufferId(id);
    return id_buffer_name_map_.insert({id, name}).second;
}

bool Material::HasBufferId(EntityId id) const
{
    return static_cast<bool>(id_buffer_name_map_.count(id));
}

bool Material::RemoveBufferId(EntityId id)
{
    if (!HasBufferId(id))
        return false;
    auto it = id_buffer_name_map_.find(id);
    id_buffer_name_map_.erase(it);
    return true;
}

std::vector<EntityId> Material::GetBufferIds() const
{
    std::vector<EntityId> vec;
    for (const auto& p : id_buffer_name_map_)
    {
        vec.push_back(p.first);
    }
    return vec;
}

} // End namespace frame::opengl.
