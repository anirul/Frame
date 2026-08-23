#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace frame::wow
{

struct M2Vertex
{
    std::array<float, 3> position = {};
    std::array<std::uint8_t, 4> bone_weights = {};
    std::array<std::uint8_t, 4> bone_indices = {};
    std::array<float, 3> normal = {};
    std::array<float, 2> texture_coordinate = {};
};

struct M2Model
{
    std::uint32_t version = 0;
    std::vector<M2Vertex> vertices = {};
    std::vector<std::uint32_t> skin_file_data_ids = {};
    std::vector<std::uint32_t> texture_file_data_ids = {};
};

struct M2Skin
{
    std::vector<std::uint16_t> vertex_lookup = {};
    std::vector<std::uint16_t> triangle_lookup = {};
};

struct StaticMesh
{
    std::uint32_t m2_version = 0;
    std::vector<float> points = {};
    std::vector<float> normals = {};
    std::vector<float> texture_coordinates = {};
    std::vector<std::uint32_t> indices = {};
    std::vector<std::uint32_t> skin_file_data_ids = {};
    std::vector<std::uint32_t> texture_file_data_ids = {};
};

M2Model ReadM2(std::span<const std::uint8_t> bytes);
M2Skin ReadSkin(std::span<const std::uint8_t> bytes);
StaticMesh BuildStaticMesh(const M2Model& model, const M2Skin& skin);

M2Model ReadM2File(const std::filesystem::path& path);
M2Skin ReadSkinFile(const std::filesystem::path& path);

std::filesystem::path FindCompanionSkinFile(
    const std::filesystem::path& m2_path, const M2Model& model);

StaticMesh LoadStaticMesh(
    const std::filesystem::path& m2_path,
    const std::filesystem::path& skin_path = {});

} // namespace frame::wow
