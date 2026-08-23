#include "frame/wow/m2_reader.h"

#include <bit>
#include <cstring>
#include <format>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace frame::wow
{
namespace
{

constexpr std::size_t kM2VertexSize = 48;
constexpr std::size_t kM2VertexCountOffset = 0x3c;
constexpr std::size_t kM2VertexOffsetOffset = 0x40;
constexpr std::size_t kM2MinimumHeaderSize = 0x48;
constexpr std::uint32_t kFirstCommonM2Version = 264;

class Reader
{
  public:
    Reader(std::span<const std::uint8_t> bytes, std::string_view description)
        : bytes_(bytes), description_(description)
    {
    }

    std::uint16_t U16(std::size_t offset) const
    {
        Require(offset, sizeof(std::uint16_t));
        return static_cast<std::uint16_t>(bytes_[offset]) |
               static_cast<std::uint16_t>(bytes_[offset + 1]) << 8;
    }

    std::uint32_t U32(std::size_t offset) const
    {
        Require(offset, sizeof(std::uint32_t));
        return static_cast<std::uint32_t>(bytes_[offset]) |
               static_cast<std::uint32_t>(bytes_[offset + 1]) << 8 |
               static_cast<std::uint32_t>(bytes_[offset + 2]) << 16 |
               static_cast<std::uint32_t>(bytes_[offset + 3]) << 24;
    }

    float Float(std::size_t offset) const
    {
        return std::bit_cast<float>(U32(offset));
    }

    std::span<const std::uint8_t> Slice(
        std::size_t offset, std::size_t size) const
    {
        Require(offset, size);
        return bytes_.subspan(offset, size);
    }

    bool HasMagic(std::size_t offset, std::string_view magic) const
    {
        if (offset > bytes_.size() || magic.size() > bytes_.size() - offset)
        {
            return false;
        }
        return std::memcmp(
                   bytes_.data() + offset, magic.data(), magic.size()) == 0;
    }

    void Require(std::size_t offset, std::size_t size) const
    {
        if (offset > bytes_.size() || size > bytes_.size() - offset)
        {
            throw std::runtime_error(
                std::format(
                    "{} is truncated at byte {} (need {} byte(s), size is {}).",
                    description_,
                    offset,
                    size,
                    bytes_.size()));
        }
    }

  private:
    std::span<const std::uint8_t> bytes_;
    std::string description_;
};

std::size_t CheckedByteSize(
    std::uint32_t count, std::size_t element_size, std::string_view description)
{
    if (count > std::numeric_limits<std::size_t>::max() / element_size)
    {
        throw std::runtime_error(
            std::format("{} has an impossible element count.", description));
    }
    return static_cast<std::size_t>(count) * element_size;
}

std::vector<std::uint8_t> ReadFile(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream)
    {
        throw std::runtime_error(
            std::format("Could not open WoW file '{}'.", path.string()));
    }
    const std::streamsize stream_size = stream.tellg();
    if (stream_size < 0)
    {
        throw std::runtime_error(
            std::format("Could not determine size of '{}'.", path.string()));
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(stream_size));
    stream.seekg(0, std::ios::beg);
    if (!bytes.empty() &&
        !stream.read(reinterpret_cast<char*>(bytes.data()), stream_size))
    {
        throw std::runtime_error(
            std::format("Could not read WoW file '{}'.", path.string()));
    }
    return bytes;
}

std::vector<std::uint32_t> ReadFileDataIds(
    std::span<const std::uint8_t> payload, std::string_view chunk_name)
{
    if (payload.size() % sizeof(std::uint32_t) != 0)
    {
        throw std::runtime_error(
            std::format(
                "Retail M2 {} chunk has a non-integral FileDataID array.",
                chunk_name));
    }
    Reader reader(payload, chunk_name);
    std::vector<std::uint32_t> result;
    result.reserve(payload.size() / sizeof(std::uint32_t));
    for (std::size_t offset = 0; offset < payload.size(); offset += 4)
    {
        result.push_back(reader.U32(offset));
    }
    return result;
}

void ParseM2Core(std::span<const std::uint8_t> core, M2Model& model)
{
    Reader reader(core, "Retail M2 MD21 payload");
    reader.Require(0, kM2MinimumHeaderSize);
    if (!reader.HasMagic(0, "MD20"))
    {
        throw std::runtime_error(
            "Retail M2 MD21 payload does not contain an MD20 model header.");
    }

    model.version = reader.U32(4);
    if (model.version < kFirstCommonM2Version)
    {
        throw std::runtime_error(
            std::format(
                "M2 version {} predates the Retail-compatible header layout.",
                model.version));
    }

    const std::uint32_t vertex_count = reader.U32(kM2VertexCountOffset);
    const std::uint32_t vertex_offset = reader.U32(kM2VertexOffsetOffset);
    const std::size_t vertices_size =
        CheckedByteSize(vertex_count, kM2VertexSize, "Retail M2 vertex array");
    reader.Require(vertex_offset, vertices_size);

    model.vertices.reserve(vertex_count);
    for (std::uint32_t index = 0; index < vertex_count; ++index)
    {
        const std::size_t offset =
            static_cast<std::size_t>(vertex_offset) + index * kM2VertexSize;
        M2Vertex vertex;
        vertex.position = {
            reader.Float(offset),
            reader.Float(offset + 4),
            reader.Float(offset + 8)};
        for (std::size_t component = 0; component < 4; ++component)
        {
            vertex.bone_weights[component] =
                reader.Slice(offset + 12 + component, 1).front();
            vertex.bone_indices[component] =
                reader.Slice(offset + 16 + component, 1).front();
        }
        vertex.normal = {
            reader.Float(offset + 20),
            reader.Float(offset + 24),
            reader.Float(offset + 28)};
        vertex.texture_coordinate = {
            reader.Float(offset + 32), reader.Float(offset + 36)};
        model.vertices.push_back(vertex);
    }
}

void AddCandidate(
    std::vector<std::filesystem::path>& candidates,
    const std::filesystem::path& candidate)
{
    if (candidate.empty())
    {
        return;
    }
    for (const auto& existing : candidates)
    {
        if (existing == candidate)
        {
            return;
        }
    }
    candidates.push_back(candidate);
}

} // namespace

M2Model ReadM2(std::span<const std::uint8_t> bytes)
{
    Reader reader(bytes, "M2 file");
    M2Model model;

    if (reader.HasMagic(0, "MD20"))
    {
        // Retained for useful diagnostics and fixtures. Retail files normally
        // use the chunked MD21 container handled below.
        ParseM2Core(bytes, model);
        return model;
    }

    std::span<const std::uint8_t> model_payload;
    std::size_t offset = 0;
    while (offset < bytes.size())
    {
        reader.Require(offset, 8);
        const std::string_view magic(
            reinterpret_cast<const char*>(bytes.data() + offset), 4);
        const std::uint32_t payload_size = reader.U32(offset + 4);
        const auto payload = reader.Slice(offset + 8, payload_size);
        if (magic == "MD21")
        {
            if (!model_payload.empty())
            {
                throw std::runtime_error("Retail M2 contains two MD21 chunks.");
            }
            model_payload = payload;
        }
        else if (magic == "SFID")
        {
            model.skin_file_data_ids = ReadFileDataIds(payload, "SFID");
        }
        else if (magic == "TXID")
        {
            model.texture_file_data_ids = ReadFileDataIds(payload, "TXID");
        }
        offset += 8 + static_cast<std::size_t>(payload_size);
    }

    if (model_payload.empty())
    {
        throw std::runtime_error(
            "File is not a current chunked Retail M2 (MD21 chunk missing).");
    }
    ParseM2Core(model_payload, model);
    return model;
}

M2Skin ReadSkin(std::span<const std::uint8_t> bytes)
{
    Reader reader(bytes, "M2 skin file");
    reader.Require(0, 48);
    if (!reader.HasMagic(0, "SKIN"))
    {
        throw std::runtime_error("Companion file does not start with SKIN.");
    }

    const std::uint32_t vertex_count = reader.U32(4);
    const std::uint32_t vertex_offset = reader.U32(8);
    const std::uint32_t triangle_count = reader.U32(12);
    const std::uint32_t triangle_offset = reader.U32(16);
    if (triangle_count % 3 != 0)
    {
        throw std::runtime_error(
            "M2 skin triangle lookup count is not divisible by three.");
    }

    reader.Require(
        vertex_offset,
        CheckedByteSize(
            vertex_count, sizeof(std::uint16_t), "SKIN vertex lookup"));
    reader.Require(
        triangle_offset,
        CheckedByteSize(
            triangle_count, sizeof(std::uint16_t), "SKIN triangle lookup"));

    M2Skin skin;
    skin.vertex_lookup.reserve(vertex_count);
    for (std::uint32_t index = 0; index < vertex_count; ++index)
    {
        skin.vertex_lookup.push_back(
            reader.U16(vertex_offset + index * sizeof(std::uint16_t)));
    }
    skin.triangle_lookup.reserve(triangle_count);
    for (std::uint32_t index = 0; index < triangle_count; ++index)
    {
        skin.triangle_lookup.push_back(
            reader.U16(triangle_offset + index * sizeof(std::uint16_t)));
    }
    return skin;
}

StaticMesh BuildStaticMesh(const M2Model& model, const M2Skin& skin)
{
    if (model.vertices.empty())
    {
        throw std::runtime_error("Retail M2 has no vertices.");
    }
    if (skin.triangle_lookup.empty())
    {
        throw std::runtime_error("Retail M2 skin has no triangles.");
    }

    StaticMesh mesh;
    mesh.m2_version = model.version;
    mesh.skin_file_data_ids = model.skin_file_data_ids;
    mesh.texture_file_data_ids = model.texture_file_data_ids;
    mesh.points.reserve(model.vertices.size() * 3);
    mesh.normals.reserve(model.vertices.size() * 3);
    mesh.texture_coordinates.reserve(model.vertices.size() * 2);
    for (const auto& vertex : model.vertices)
    {
        // WoW is Z-up. Frame's assets use +Y up; this rotation preserves
        // handedness, so the SKIN winding does not need to be reversed.
        mesh.points.insert(
            mesh.points.end(),
            {vertex.position[0], vertex.position[2], -vertex.position[1]});
        mesh.normals.insert(
            mesh.normals.end(),
            {vertex.normal[0], vertex.normal[2], -vertex.normal[1]});
        mesh.texture_coordinates.insert(
            mesh.texture_coordinates.end(),
            {vertex.texture_coordinate[0], vertex.texture_coordinate[1]});
    }

    mesh.indices.reserve(skin.triangle_lookup.size());
    for (const std::uint16_t lookup_index : skin.triangle_lookup)
    {
        if (lookup_index >= skin.vertex_lookup.size())
        {
            throw std::runtime_error(
                std::format(
                    "SKIN triangle lookup {} exceeds {} lookup vertices.",
                    lookup_index,
                    skin.vertex_lookup.size()));
        }
        const std::uint16_t vertex_index = skin.vertex_lookup[lookup_index];
        if (vertex_index >= model.vertices.size())
        {
            throw std::runtime_error(
                std::format(
                    "SKIN vertex {} exceeds {} M2 vertices.",
                    vertex_index,
                    model.vertices.size()));
        }
        mesh.indices.push_back(vertex_index);
    }
    return mesh;
}

M2Model ReadM2File(const std::filesystem::path& path)
{
    return ReadM2(ReadFile(path));
}

M2Skin ReadSkinFile(const std::filesystem::path& path)
{
    return ReadSkin(ReadFile(path));
}

std::filesystem::path FindCompanionSkinFile(
    const std::filesystem::path& m2_path, const M2Model& model)
{
    std::vector<std::filesystem::path> candidates;
    auto same_stem = m2_path;
    same_stem.replace_extension(".skin");
    AddCandidate(candidates, same_stem);
    AddCandidate(
        candidates,
        m2_path.parent_path() /
            std::filesystem::path(m2_path.stem().string() + "00.skin"));

    if (!model.skin_file_data_ids.empty())
    {
        const std::string id = std::to_string(model.skin_file_data_ids.front());
        AddCandidate(candidates, m2_path.parent_path() / (id + ".skin"));
        AddCandidate(candidates, m2_path.parent_path() / id);
    }

    for (const auto& candidate : candidates)
    {
        if (std::filesystem::is_regular_file(candidate))
        {
            return candidate;
        }
    }

    std::string checked;
    for (const auto& candidate : candidates)
    {
        if (!checked.empty())
        {
            checked += ", ";
        }
        checked += candidate.string();
    }
    throw std::runtime_error(
        std::format(
            "Could not find the extracted Retail SKIN companion for '{}'. "
            "Checked: {}.",
            m2_path.string(),
            checked));
}

StaticMesh LoadStaticMesh(
    const std::filesystem::path& m2_path,
    const std::filesystem::path& skin_path)
{
    const M2Model model = ReadM2File(m2_path);
    const auto resolved_skin =
        skin_path.empty() ? FindCompanionSkinFile(m2_path, model) : skin_path;
    return BuildStaticMesh(model, ReadSkinFile(resolved_skin));
}

} // namespace frame::wow
