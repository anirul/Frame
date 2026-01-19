#include "frame/vulkan/material.h"

#include <algorithm>
#include <stdexcept>

#include "frame/level_interface.h"

namespace frame::vulkan
{

EntityId Material::GetProgramId(const LevelInterface* level) const
{
    if (program_id_ != NullId || !level)
    {
        return program_id_;
    }
    const auto& proto = GetData();
    if (proto.program_name().empty())
    {
        return NullId;
    }
    return level->GetIdFromName(proto.program_name());
}

EntityId Material::GetPreprocessProgramId(const LevelInterface* level) const
{
    if (preprocess_program_id_ != NullId || !level)
    {
        return preprocess_program_id_;
    }
    const auto& proto = GetData();
    if (proto.preprocess_program_name().empty())
    {
        return NullId;
    }
    return level->GetIdFromName(proto.preprocess_program_name());
}

std::string Material::GetInnerName(EntityId id) const
{
    if (!texture_map_.count(id))
    {
        throw std::runtime_error("Unknown texture id.");
    }
    return texture_map_.at(id).first;
}

void Material::SetProgramId(EntityId id)
{
    program_id_ = id;
}

void Material::SetPreprocessProgramId(EntityId id)
{
    preprocess_program_id_ = id;
}

bool Material::AddTextureId(EntityId id, const std::string& name)
{
    const int slot = static_cast<int>(texture_order_.size());
    auto [it, inserted] =
        texture_map_.insert({id, {name, slot}});
    if (!inserted)
    {
        return false;
    }
    texture_order_.push_back(id);
    return true;
}

bool Material::HasTextureId(EntityId id) const
{
    return texture_map_.count(id) != 0;
}

bool Material::RemoveTextureId(EntityId id)
{
    if (!HasTextureId(id))
    {
        return false;
    }
    texture_map_.erase(id);
    texture_order_.erase(
        std::remove(texture_order_.begin(), texture_order_.end(), id),
        texture_order_.end());
    return true;
}

std::vector<EntityId> Material::GetTextureIds() const
{
    return texture_order_;
}

std::string Material::GetInnerBufferName(const std::string& name) const
{
    auto it = std::find_if(
        buffer_names_.begin(),
        buffer_names_.end(),
        [&name](const auto& entry) { return entry.first == name; });
    if (it == buffer_names_.end())
    {
        throw std::runtime_error("Unknown buffer name.");
    }
    return it->second;
}

bool Material::AddBufferName(
    const std::string& name, const std::string& inner_name)
{
    auto it = std::find_if(
        buffer_names_.begin(),
        buffer_names_.end(),
        [&name](const auto& entry) { return entry.first == name; });
    if (it != buffer_names_.end())
    {
        return false;
    }
    buffer_names_.emplace_back(name, inner_name);
    return true;
}

std::vector<std::string> Material::GetBufferNames() const
{
    std::vector<std::string> names;
    names.reserve(buffer_names_.size());
    for (const auto& entry : buffer_names_)
    {
        names.push_back(entry.first);
    }
    return names;
}

std::string Material::GetInnerNodeName(const std::string& name) const
{
    auto it = std::find_if(
        node_names_.begin(),
        node_names_.end(),
        [&name](const auto& entry) { return entry.first == name; });
    if (it == node_names_.end())
    {
        throw std::runtime_error("Unknown node name.");
    }
    return it->second;
}

bool Material::AddNodeName(
    const std::string& name, const std::string& inner_name)
{
    auto it = std::find_if(
        node_names_.begin(),
        node_names_.end(),
        [&name](const auto& entry) { return entry.first == name; });
    if (it != node_names_.end())
    {
        return false;
    }
    node_names_.emplace_back(name, inner_name);
    return true;
}

std::vector<std::string> Material::GetNodeNames() const
{
    std::vector<std::string> names;
    names.reserve(node_names_.size());
    for (const auto& entry : node_names_)
    {
        names.push_back(entry.first);
    }
    return names;
}

std::pair<std::string, int> Material::EnableTextureId(EntityId id) const
{
    if (!HasTextureId(id))
    {
        throw std::runtime_error("Texture id not registered.");
    }
    const auto& entry = texture_map_.at(id);
    return entry;
}

void Material::DisableTextureId(EntityId /*id*/) const
{
    // CPU backed implementation does not bind resources.
}

} // namespace frame::vulkan
