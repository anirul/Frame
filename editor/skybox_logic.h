#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

#include "frame/json/proto.h"
#include "frame/level_interface.h"

namespace frame::gui::skybox
{

using CubemapSnapshots = std::unordered_map<std::string, frame::proto::Texture>;

namespace detail
{

inline void AppendUnique(
    std::vector<std::string>& values, const std::string& value)
{
    if (value.empty())
    {
        return;
    }
    if (std::find(values.begin(), values.end(), value) == values.end())
    {
        values.push_back(value);
    }
}

inline bool Contains(
    const std::vector<std::string>& values, const std::string& value)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}

inline const frame::proto::Texture* FindTextureByName(
    const frame::proto::Level& proto_level, const std::string& texture_name)
{
    for (const auto& texture : proto_level.textures())
    {
        if (texture.name() == texture_name)
        {
            return &texture;
        }
    }
    return nullptr;
}

inline const frame::proto::Program* FindProgramByName(
    const frame::proto::Level& proto_level, const std::string& program_name)
{
    for (const auto& program : proto_level.programs())
    {
        if (program.name() == program_name)
        {
            return &program;
        }
    }
    return nullptr;
}

inline bool AreCubemapSourcesEquivalent(
    const frame::proto::Texture& a, const frame::proto::Texture& b)
{
    if (!a.cubemap() || !b.cubemap())
    {
        return false;
    }
    if (a.has_file_name() && b.has_file_name())
    {
        return a.file_name() == b.file_name();
    }
    if (a.has_file_names() && b.has_file_names())
    {
        return a.file_names().positive_x() == b.file_names().positive_x() &&
               a.file_names().negative_x() == b.file_names().negative_x() &&
               a.file_names().positive_y() == b.file_names().positive_y() &&
               a.file_names().negative_y() == b.file_names().negative_y() &&
               a.file_names().positive_z() == b.file_names().positive_z() &&
               a.file_names().negative_z() == b.file_names().negative_z();
    }
    return false;
}

} // namespace detail

inline void CaptureCubemapSnapshots(
    const frame::proto::Level& proto_level,
    CubemapSnapshots& snapshots)
{
    for (const auto& texture : proto_level.textures())
    {
        if (!texture.cubemap() || texture.name().empty())
        {
            continue;
        }
        snapshots.try_emplace(texture.name(), texture);
    }
}

inline std::vector<std::string> CollectCubemapTextureNames(
    const LevelInterface& level)
{
    std::vector<std::string> names = {};
    for (const auto texture_id : level.GetTextures())
    {
        const auto& texture = level.GetTextureFromId(texture_id);
        if (!texture.GetData().cubemap())
        {
            continue;
        }
        names.push_back(texture.GetName());
    }
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    return names;
}

inline std::vector<std::string> CollectSkyboxDestinationTextureNames(
    const frame::proto::Level& proto_level)
{
    std::vector<std::string> destination_names = {};
    for (const auto& pass : proto_level.render_pass_programs())
    {
        if (pass.render_time_enum() != frame::proto::NodeMesh::SKYBOX_RENDER_TIME)
        {
            continue;
        }
        const auto* program =
            detail::FindProgramByName(proto_level, pass.program_name());
        if (!program)
        {
            continue;
        }
        for (const auto& input_texture_name : program->input_texture_names())
        {
            detail::AppendUnique(destination_names, input_texture_name);
        }
    }

    // Fallback for incomplete pass setup: infer from program cubemap inputs.
    if (destination_names.empty())
    {
        for (const auto& program : proto_level.programs())
        {
            for (const auto& input_texture_name : program.input_texture_names())
            {
                const auto* texture =
                    detail::FindTextureByName(proto_level, input_texture_name);
                if (texture && texture->cubemap())
                {
                    detail::AppendUnique(destination_names, input_texture_name);
                }
            }
        }
    }

    // Keep all aliases that currently point to the same cubemap source.
    std::vector<std::string> expanded_names = destination_names;
    for (const auto& destination_name : destination_names)
    {
        const auto* destination_texture =
            detail::FindTextureByName(proto_level, destination_name);
        if (!destination_texture || !destination_texture->cubemap())
        {
            continue;
        }
        for (const auto& texture : proto_level.textures())
        {
            if (texture.name().empty() || !texture.cubemap())
            {
                continue;
            }
            if (detail::AreCubemapSourcesEquivalent(*destination_texture, texture))
            {
                detail::AppendUnique(expanded_names, texture.name());
            }
        }
    }
    return expanded_names;
}

inline std::optional<frame::proto::Texture> ResolveCubemapSourceTexture(
    const frame::proto::Level& proto_level,
    const CubemapSnapshots& snapshots,
    const std::string& cubemap_name,
    const std::vector<std::string>& destination_names)
{
    if (cubemap_name.empty())
    {
        return std::nullopt;
    }

    // For destination aliases, use immutable source snapshots to avoid selecting
    // a texture that was overwritten by a previous skybox apply.
    if (!detail::Contains(destination_names, cubemap_name))
    {
        if (const auto* source =
                detail::FindTextureByName(proto_level, cubemap_name);
            source && source->cubemap())
        {
            return *source;
        }
    }

    if (const auto it = snapshots.find(cubemap_name); it != snapshots.end())
    {
        return it->second;
    }

    if (const auto* source = detail::FindTextureByName(proto_level, cubemap_name);
        source && source->cubemap())
    {
        return *source;
    }

    return std::nullopt;
}

inline std::optional<std::string> FindCurrentSkyboxSourceName(
    const LevelInterface& level,
    const std::vector<std::string>& cubemap_names,
    const std::vector<std::string>& destination_names)
{
    const frame::proto::Texture* current_skybox_texture = nullptr;
    for (const auto& destination_name : destination_names)
    {
        const auto destination_id = level.GetIdFromName(destination_name);
        if (destination_id == frame::NullId)
        {
            continue;
        }
        const auto& texture_data =
            level.GetTextureFromId(destination_id).GetData();
        if (!texture_data.cubemap())
        {
            continue;
        }
        current_skybox_texture = &texture_data;
        break;
    }
    if (!current_skybox_texture)
    {
        return std::nullopt;
    }

    for (const auto& candidate_name : cubemap_names)
    {
        const auto candidate_id = level.GetIdFromName(candidate_name);
        if (candidate_id == frame::NullId)
        {
            continue;
        }
        const auto& candidate_texture =
            level.GetTextureFromId(candidate_id).GetData();
        if (detail::AreCubemapSourcesEquivalent(
                *current_skybox_texture,
                candidate_texture))
        {
            return candidate_name;
        }
    }
    return std::nullopt;
}

} // namespace frame::gui::skybox
