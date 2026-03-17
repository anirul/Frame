#include "frame/opengl/raytracing_test.h"

#include <algorithm>
#include <array>

namespace test
{

namespace
{

frame::EntityId FindMaterialForNode(
    const frame::LevelInterface& level,
    frame::proto::NodeMesh::RenderTimeEnum render_time_enum,
    const std::string& node_name)
{
    for (const auto& [node_id, material_id] : level.GetMeshMaterialIds(render_time_enum))
    {
        if (level.GetNameFromId(node_id) == node_name)
        {
            return material_id;
        }
    }
    return frame::NullId;
}

frame::EntityId FindTextureByInnerName(
    const frame::MaterialInterface& material,
    const std::string& expected_inner_name)
{
    for (const auto texture_id : material.GetTextureIds())
    {
        if (material.GetInnerName(texture_id) == expected_inner_name)
        {
            return texture_id;
        }
    }
    return frame::NullId;
}

float ReadTextureFirstChannel(
    const frame::LevelInterface& level, frame::EntityId texture_id)
{
    if (!texture_id)
    {
        return 0.0f;
    }

    const auto& texture = level.GetTextureFromId(texture_id);
    switch (texture.GetData().pixel_element_size().value())
    {
    case frame::proto::PixelElementSize::FLOAT: {
        const auto data = texture.GetTextureFloat();
        return data.empty() ? 0.0f : data.front();
    }
    case frame::proto::PixelElementSize::SHORT:
    case frame::proto::PixelElementSize::HALF: {
        const auto data = texture.GetTextureWord();
        return data.empty() ? 0.0f
                            : static_cast<float>(data.front()) / 65535.0f;
    }
    case frame::proto::PixelElementSize::BYTE:
    default: {
        const auto data = texture.GetTextureByte();
        return data.empty() ? 0.0f
                            : static_cast<float>(data.front()) / 255.0f;
    }
    }
}

std::array<float, 3> ReadTextureRgb(
    const frame::LevelInterface& level, frame::EntityId texture_id)
{
    if (!texture_id)
    {
        return {0.0f, 0.0f, 0.0f};
    }

    const auto& texture = level.GetTextureFromId(texture_id);
    switch (texture.GetData().pixel_element_size().value())
    {
    case frame::proto::PixelElementSize::FLOAT: {
        const auto data = texture.GetTextureFloat();
        if (data.size() < 3)
        {
            return {0.0f, 0.0f, 0.0f};
        }
        return {data[0], data[1], data[2]};
    }
    case frame::proto::PixelElementSize::SHORT:
    case frame::proto::PixelElementSize::HALF: {
        const auto data = texture.GetTextureWord();
        if (data.size() < 3)
        {
            return {0.0f, 0.0f, 0.0f};
        }
        return {
            static_cast<float>(data[0]) / 65535.0f,
            static_cast<float>(data[1]) / 65535.0f,
            static_cast<float>(data[2]) / 65535.0f};
    }
    case frame::proto::PixelElementSize::BYTE:
    default: {
        const auto data = texture.GetTextureByte();
        if (data.size() < 3)
        {
            return {0.0f, 0.0f, 0.0f};
        }
        return {
            static_cast<float>(data[0]) / 255.0f,
            static_cast<float>(data[1]) / 255.0f,
            static_cast<float>(data[2]) / 255.0f};
    }
    }
}

bool HasInnerBuffer(
    const frame::MaterialInterface& material,
    const std::string& expected_inner_name)
{
    for (const auto& buffer_name : material.GetBufferNames())
    {
        if (material.GetInnerBufferName(buffer_name) == expected_inner_name)
        {
            return true;
        }
    }
    return false;
}

bool HasNodeBinding(
    const frame::MaterialInterface& material,
    const std::string& node_name,
    const std::string& inner_name)
{
    const auto node_names = material.GetNodeNames();
    return std::find(node_names.begin(), node_names.end(), node_name) !=
               node_names.end() &&
           material.GetInnerNodeName(node_name) == inner_name;
}

} // namespace

TEST_F(OpenGLRayTracingLevelTest, RaytracingLevelBindsSceneTriangleBuffers)
{
    auto level = LoadLevel("asset/json/raytracing.json");
    ASSERT_NE(level, nullptr);

    const auto material_id = level->GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    const auto& material = level->GetMaterialFromId(material_id);
    EXPECT_TRUE(HasInnerBuffer(material, "TriangleBufferTransmissive"));
    EXPECT_TRUE(HasInnerBuffer(material, "TriangleBufferOpaque"));
}

TEST_F(OpenGLRayTracingLevelTest, DragonLevelBindsSceneBvhAndModel)
{
    auto level = LoadLevel("asset/json/dragon.json");
    ASSERT_NE(level, nullptr);

    const auto material_id = level->GetIdFromName("RayTraceMaterial");
    ASSERT_NE(material_id, frame::NullId);
    const auto& material = level->GetMaterialFromId(material_id);
    EXPECT_TRUE(HasInnerBuffer(material, "TriangleBuffer"));
    EXPECT_TRUE(HasInnerBuffer(material, "BvhBuffer"));
    EXPECT_TRUE(HasNodeBinding(material, "DragonMesh", "model"));
}

TEST_F(OpenGLRayTracingLevelTest, ImportsGltfGlassMaterialFromIco)
{
    auto level = LoadLevel("asset/json/raytracing.json");
    ASSERT_NE(level, nullptr);

    const auto material_id = FindMaterialForNode(
        *level,
        frame::proto::NodeMesh::PRE_RENDER_TIME,
        "Ico");
    ASSERT_NE(material_id, frame::NullId);
    const auto& material = level->GetMaterialFromId(material_id);

    const auto transmission_texture_id = FindTextureByInnerName(
        material, "transmission_texture");
    const auto ior_texture_id = FindTextureByInnerName(material, "ior_texture");
    const auto thickness_texture_id = FindTextureByInnerName(
        material, "thickness_texture");
    const auto attenuation_color_texture_id = FindTextureByInnerName(
        material, "attenuation_color_texture");
    const auto attenuation_distance_texture_id = FindTextureByInnerName(
        material, "attenuation_distance_texture");

    ASSERT_NE(transmission_texture_id, frame::NullId);
    ASSERT_NE(ior_texture_id, frame::NullId);
    ASSERT_NE(thickness_texture_id, frame::NullId);
    ASSERT_NE(attenuation_color_texture_id, frame::NullId);
    ASSERT_NE(attenuation_distance_texture_id, frame::NullId);

    EXPECT_NEAR(
        ReadTextureFirstChannel(*level, transmission_texture_id),
        1.0f,
        0.02f);
    EXPECT_NEAR(
        ReadTextureFirstChannel(*level, ior_texture_id),
        1.45f,
        0.02f);
    EXPECT_NEAR(
        ReadTextureFirstChannel(*level, thickness_texture_id),
        0.35f,
        0.02f);
    EXPECT_NEAR(
        ReadTextureFirstChannel(*level, attenuation_distance_texture_id),
        1.5f,
        0.02f);

    const auto attenuation_color = ReadTextureRgb(
        *level, attenuation_color_texture_id);
    EXPECT_NEAR(attenuation_color[0], 0.82f, 0.03f);
    EXPECT_NEAR(attenuation_color[1], 0.97f, 0.03f);
    EXPECT_NEAR(attenuation_color[2], 0.90f, 0.03f);
}

} // namespace test
