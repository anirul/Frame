#include "frame/wow/m2_reader.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

namespace test
{
namespace
{

void WriteU16(
    std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint16_t value)
{
    ASSERT_LE(offset + 2, bytes.size());
    bytes[offset] = static_cast<std::uint8_t>(value);
    bytes[offset + 1] = static_cast<std::uint8_t>(value >> 8);
}

void WriteU32(
    std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value)
{
    ASSERT_LE(offset + 4, bytes.size());
    for (std::size_t byte = 0; byte < 4; ++byte)
    {
        bytes[offset + byte] = static_cast<std::uint8_t>(value >> (byte * 8));
    }
}

void WriteFloat(
    std::vector<std::uint8_t>& bytes, std::size_t offset, float value)
{
    WriteU32(bytes, offset, std::bit_cast<std::uint32_t>(value));
}

void WriteMagic(
    std::vector<std::uint8_t>& bytes, std::size_t offset, const char* magic)
{
    ASSERT_LE(offset + 4, bytes.size());
    for (std::size_t index = 0; index < 4; ++index)
    {
        bytes[offset + index] = static_cast<std::uint8_t>(magic[index]);
    }
}

void AppendChunk(
    std::vector<std::uint8_t>& file,
    const char* magic,
    const std::vector<std::uint8_t>& payload)
{
    const std::size_t start = file.size();
    file.resize(start + 8 + payload.size());
    WriteMagic(file, start, magic);
    WriteU32(file, start + 4, static_cast<std::uint32_t>(payload.size()));
    std::copy(payload.begin(), payload.end(), file.begin() + start + 8);
}

std::vector<std::uint8_t> MakeM2()
{
    constexpr std::size_t kVertexOffset = 0x80;
    constexpr std::size_t kVertexSize = 48;
    std::vector<std::uint8_t> core(kVertexOffset + 3 * kVertexSize);
    WriteMagic(core, 0, "MD20");
    WriteU32(core, 4, 274);
    WriteU32(core, 0x3c, 3);
    WriteU32(core, 0x40, kVertexOffset);

    const auto write_vertex =
        [&](std::size_t index, float x, float y, float z, float u, float v) {
            const std::size_t offset = kVertexOffset + index * kVertexSize;
            WriteFloat(core, offset, x);
            WriteFloat(core, offset + 4, y);
            WriteFloat(core, offset + 8, z);
            core[offset + 12] = 255;
            WriteFloat(core, offset + 20, 0.0f);
            WriteFloat(core, offset + 24, 1.0f);
            WriteFloat(core, offset + 28, 0.0f);
            WriteFloat(core, offset + 32, u);
            WriteFloat(core, offset + 36, v);
        };
    write_vertex(0, 1.0f, 2.0f, 3.0f, 0.0f, 0.0f);
    write_vertex(1, 4.0f, 5.0f, 6.0f, 1.0f, 0.0f);
    write_vertex(2, 7.0f, 8.0f, 9.0f, 0.0f, 1.0f);

    std::vector<std::uint8_t> sfid(8);
    WriteU32(sfid, 0, 123456);
    WriteU32(sfid, 4, 654321);
    std::vector<std::uint8_t> txid(4);
    WriteU32(txid, 0, 777777);

    std::vector<std::uint8_t> file;
    AppendChunk(file, "MD21", core);
    AppendChunk(file, "SFID", sfid);
    AppendChunk(file, "TXID", txid);
    return file;
}

std::vector<std::uint8_t> MakeSkin()
{
    constexpr std::size_t kVertexOffset = 48;
    constexpr std::size_t kTriangleOffset = kVertexOffset + 6;
    std::vector<std::uint8_t> skin(kTriangleOffset + 6);
    WriteMagic(skin, 0, "SKIN");
    WriteU32(skin, 4, 3);
    WriteU32(skin, 8, kVertexOffset);
    WriteU32(skin, 12, 3);
    WriteU32(skin, 16, kTriangleOffset);
    WriteU16(skin, kVertexOffset, 2);
    WriteU16(skin, kVertexOffset + 2, 0);
    WriteU16(skin, kVertexOffset + 4, 1);
    WriteU16(skin, kTriangleOffset, 1);
    WriteU16(skin, kTriangleOffset + 2, 2);
    WriteU16(skin, kTriangleOffset + 4, 0);
    return skin;
}

} // namespace

TEST(M2ReaderTest, ReadsChunkedRetailBindPose)
{
    const auto model = frame::wow::ReadM2(MakeM2());
    const auto skin = frame::wow::ReadSkin(MakeSkin());
    const auto mesh = frame::wow::BuildStaticMesh(model, skin);

    EXPECT_EQ(274, mesh.m2_version);
    EXPECT_EQ(
        (std::vector<std::uint32_t>{123456, 654321}), mesh.skin_file_data_ids);
    EXPECT_EQ((std::vector<std::uint32_t>{777777}), mesh.texture_file_data_ids);
    EXPECT_EQ((std::vector<std::uint32_t>{0, 1, 2}), mesh.indices);
    ASSERT_EQ(9, mesh.points.size());
    EXPECT_FLOAT_EQ(1.0f, mesh.points[0]);
    EXPECT_FLOAT_EQ(3.0f, mesh.points[1]);
    EXPECT_FLOAT_EQ(-2.0f, mesh.points[2]);
    ASSERT_EQ(9, mesh.normals.size());
    EXPECT_FLOAT_EQ(0.0f, mesh.normals[0]);
    EXPECT_FLOAT_EQ(0.0f, mesh.normals[1]);
    EXPECT_FLOAT_EQ(-1.0f, mesh.normals[2]);
    EXPECT_EQ(
        (std::vector<float>{0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f}),
        mesh.texture_coordinates);
}

TEST(M2ReaderTest, RejectsOutOfRangeSkinLookup)
{
    auto skin_bytes = MakeSkin();
    WriteU16(skin_bytes, 54, 3);
    const auto model = frame::wow::ReadM2(MakeM2());
    const auto skin = frame::wow::ReadSkin(skin_bytes);
    EXPECT_THROW(frame::wow::BuildStaticMesh(model, skin), std::runtime_error);
}

TEST(M2ReaderTest, RejectsTruncatedRetailChunk)
{
    auto bytes = MakeM2();
    bytes.pop_back();
    EXPECT_THROW(frame::wow::ReadM2(bytes), std::runtime_error);
}

} // namespace test
