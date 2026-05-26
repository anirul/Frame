#include "frame/json/parse_level_test.h"

#include <algorithm>

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"
#include "frame/json/program_key.h"
#include "frame/vulkan/build_level.h"
#include "frame/vulkan/json/parse_level.h"
#include "frame/vulkan/skinned_mesh.h"

namespace test
{

TEST_F(ParseLevelTest, BuildVulkanLevelFromJson)
{
    const auto asset_root = frame::file::FindDirectory("asset");
    const auto level_path = frame::file::FindFile("asset/json/level_test.json");
    const auto level_proto = frame::json::LoadLevelProto(level_path);
    const auto level_data = frame::json::ParseLevelData(
        glm::uvec2(320, 200), level_path, asset_root);

    auto built = frame::vulkan::BuildLevel(glm::uvec2(320, 200), level_data);
    ASSERT_NE(built.level, nullptr);
    EXPECT_EQ(
        built.level->GetDefaultRootSceneNodeId(),
        built.level->GetIdFromName(
            level_proto.scene_tree().default_root_name()));
}

TEST_F(ParseLevelTest, CreateLevelDataFromPath)
{
    const auto asset_root = frame::file::FindDirectory("asset");
    const auto level_data = frame::json::ParseLevelData(
        glm::uvec2(320, 200),
        frame::file::FindFile("asset/json/level_test.json"),
        asset_root);
    EXPECT_EQ(level_data.proto.name(), "LevelTest");
    ASSERT_EQ(level_data.textures.size(), 2u);
    EXPECT_EQ(level_data.textures.front().name, "skybox");
    EXPECT_EQ(level_data.textures.back().name, "output");
    EXPECT_EQ(level_data.asset_root, asset_root);
}

TEST_F(ParseLevelTest, CreateLevelDataFromPathVulkan)
{
    const auto asset_root = frame::file::FindDirectory("asset");
    const auto level_data = frame::vulkan::json::ParseLevelData(
        glm::uvec2(320, 200),
        frame::file::FindFile("asset/json/level_test.json"),
        asset_root);
    EXPECT_EQ(level_data.proto.name(), "LevelTest");
    ASSERT_EQ(level_data.textures.size(), 2u);
    EXPECT_EQ(level_data.textures.front().name, "skybox");
    EXPECT_EQ(level_data.textures.back().name, "output");
    EXPECT_EQ(level_data.asset_root, asset_root);
}

TEST_F(ParseLevelTest, RasterOptionBuildsRasterSceneProgramForRaytracingJson)
{
    const auto asset_root = frame::file::FindDirectory("asset");
    const frame::json::LevelDataOptions options{
        .render_preset = frame::json::RenderPreset::Raster};
    const auto level_data = frame::json::ParseLevelData(
        glm::uvec2(320, 200),
        frame::file::FindFile("asset/json/raytracing.json"),
        asset_root,
        options);

    const auto has_node = [&](const std::string& name) {
        return std::find_if(
                   level_data.proto.scene_tree().node_meshes().begin(),
                   level_data.proto.scene_tree().node_meshes().end(),
                   [&](const frame::proto::NodeMesh& node) {
                       return node.name() == name;
                   }) != level_data.proto.scene_tree().node_meshes().end();
    };
    EXPECT_FALSE(has_node("RayTracingRendering"));
    EXPECT_TRUE(
        std::all_of(
            level_data.proto.scene_tree().node_meshes().begin(),
            level_data.proto.scene_tree().node_meshes().end(),
            [](const frame::proto::NodeMesh& node) {
                return node.acceleration_structure_enum() ==
                       frame::proto::NodeMesh::NO_ACCELERATION;
            }));

    const auto raster_program = std::find_if(
        level_data.programs.begin(),
        level_data.programs.end(),
        [](const frame::json::ProgramInfo& program) {
            return program.name == "RasterSceneProgram";
        });
    ASSERT_NE(raster_program, level_data.programs.end());
    EXPECT_FALSE(
        frame::json::IsRaytracingProgramKey(
            frame::json::ResolveProgramKey(raster_program->proto)));
    const auto has_binding = [&](const std::string& name) {
        return std::find_if(
                   raster_program->proto.bindings().begin(),
                   raster_program->proto.bindings().end(),
                   [&](const frame::proto::ProgramBinding& binding) {
                       return binding.name() == name &&
                              binding.binding_type() ==
                                  frame::proto::ProgramBinding::
                                      COMBINED_IMAGE_SAMPLER;
                   }) != raster_program->proto.bindings().end();
    };
    EXPECT_TRUE(has_binding("albedo_texture"));
    EXPECT_TRUE(has_binding("normal_texture"));
    EXPECT_TRUE(has_binding("roughness_texture"));
    EXPECT_TRUE(has_binding("metallic_texture"));
    EXPECT_TRUE(has_binding("ao_texture"));
    EXPECT_TRUE(has_binding("specular_factor_texture"));
    EXPECT_TRUE(has_binding("specular_color_texture"));
    EXPECT_TRUE(has_binding("shadow_map"));
    EXPECT_TRUE(has_binding("transmission_texture"));
    EXPECT_TRUE(has_binding("ior_texture"));
    EXPECT_TRUE(has_binding("thickness_texture"));
    EXPECT_TRUE(has_binding("attenuation_color_texture"));
    EXPECT_TRUE(has_binding("skybox_env"));
    EXPECT_FALSE(
        std::any_of(
            level_data.programs.begin(),
            level_data.programs.end(),
            [](const frame::json::ProgramInfo& program) {
                return frame::json::IsRaytracingProgramKey(
                    frame::json::ResolveProgramKey(program.proto));
            }));

    const auto has_pass =
        [&](frame::proto::NodeMesh::RenderTimeEnum render_time,
            const std::string& program_name) {
            return std::find_if(
                       level_data.render_pass_programs.begin(),
                       level_data.render_pass_programs.end(),
                       [&](const frame::json::RenderPassProgramInfo& pass) {
                           return pass.render_time == render_time &&
                                  pass.program_name == program_name;
                       }) != level_data.render_pass_programs.end();
        };
    EXPECT_TRUE(has_pass(
        frame::proto::NodeMesh::SCENE_RENDER_TIME, "RasterSceneProgram"));
    EXPECT_TRUE(
        has_pass(frame::proto::NodeMesh::SKYBOX_RENDER_TIME, "CubemapProgram"));

    auto built = frame::vulkan::BuildLevel(glm::uvec2(320, 200), level_data);
    ASSERT_NE(built.level, nullptr);
    const auto scene_program_id = built.level->GetRenderPassProgramId(
        frame::proto::NodeMesh::SCENE_RENDER_TIME);
    ASSERT_NE(scene_program_id, frame::NullId);
    EXPECT_EQ(
        built.level->GetNameFromId(scene_program_id), "RasterSceneProgram");
    EXPECT_EQ(
        built.level->GetRenderPassPreprocessProgramId(
            frame::proto::NodeMesh::SCENE_RENDER_TIME),
        frame::NullId);
    for (const auto& [node_id, material_id] : built.level->GetMeshMaterialIds(
             frame::proto::NodeMesh::SCENE_RENDER_TIME))
    {
        if (material_id != frame::NullId)
        {
            const auto& material = built.level->GetMaterialFromId(material_id);
            bool has_specular_factor = false;
            bool has_specular_color = false;
            for (const auto texture_id : material.GetTextureIds())
            {
                has_specular_factor |= material.GetInnerName(texture_id) ==
                                       "specular_factor_texture";
                has_specular_color |= material.GetInnerName(texture_id) ==
                                      "specular_color_texture";
            }
            EXPECT_TRUE(has_specular_factor);
            EXPECT_TRUE(has_specular_color);
        }
        const auto mesh_id =
            built.level->GetSceneNodeFromId(node_id).GetLocalMesh();
        if (mesh_id == frame::NullId)
        {
            continue;
        }
        EXPECT_EQ(
            built.level->GetMeshFromId(mesh_id).GetBvhBufferId(),
            frame::NullId);
    }
}

TEST_F(ParseLevelTest, RasterOptionKeepsSkinnedMeshAnimatedWithoutBvh)
{
    const auto asset_root = frame::file::FindDirectory("asset");
    const frame::json::LevelDataOptions options{
        .render_preset = frame::json::RenderPreset::Raster};
    const auto level_data = frame::json::ParseLevelData(
        glm::uvec2(320, 200),
        frame::file::FindFile("asset/json/skinned_mesh.json"),
        asset_root,
        options);

    const auto has_node = [&](const std::string& name) {
        return std::find_if(
                   level_data.proto.scene_tree().node_meshes().begin(),
                   level_data.proto.scene_tree().node_meshes().end(),
                   [&](const frame::proto::NodeMesh& node) {
                       return node.name() == name;
                   }) != level_data.proto.scene_tree().node_meshes().end();
    };
    EXPECT_TRUE(has_node("CesiumManMesh"));
    EXPECT_FALSE(has_node("RayTracingRendering"));
    EXPECT_TRUE(
        std::all_of(
            level_data.proto.scene_tree().node_meshes().begin(),
            level_data.proto.scene_tree().node_meshes().end(),
            [](const frame::proto::NodeMesh& node) {
                return node.acceleration_structure_enum() ==
                       frame::proto::NodeMesh::NO_ACCELERATION;
            }));

    const auto scene_pass = std::find_if(
        level_data.render_pass_programs.begin(),
        level_data.render_pass_programs.end(),
        [](const frame::json::RenderPassProgramInfo& pass) {
            return pass.render_time ==
                   frame::proto::NodeMesh::SCENE_RENDER_TIME;
        });
    ASSERT_NE(scene_pass, level_data.render_pass_programs.end());
    EXPECT_EQ(scene_pass->program_name, "RasterSceneProgram");
    EXPECT_TRUE(scene_pass->preprocess_program_name.empty());
    EXPECT_FALSE(
        std::any_of(
            level_data.programs.begin(),
            level_data.programs.end(),
            [](const frame::json::ProgramInfo& program) {
                return frame::json::IsRaytracingProgramKey(
                    frame::json::ResolveProgramKey(program.proto));
            }));

    auto built = frame::vulkan::BuildLevel(glm::uvec2(320, 200), level_data);
    ASSERT_NE(built.level, nullptr);
    auto& level = *built.level;
    const auto mesh_node_id = level.GetIdFromName("CesiumManMesh");
    ASSERT_NE(mesh_node_id, frame::NullId);
    const auto mesh_id = level.GetSceneNodeFromId(mesh_node_id).GetLocalMesh();
    ASSERT_NE(mesh_id, frame::NullId);
    auto* skinned_mesh = dynamic_cast<frame::vulkan::SkinnedMesh*>(
        &level.GetMeshFromId(mesh_id));
    ASSERT_NE(skinned_mesh, nullptr);
    EXPECT_TRUE(skinned_mesh->IsSkinningAnimationEnabled());
    EXPECT_TRUE(skinned_mesh->HasBoneMatricesCallback());
    EXPECT_TRUE(skinned_mesh->HasGpuSkinningSourceData());
    EXPECT_TRUE(skinned_mesh->HasRaytraceTriangleCallback());
    EXPECT_FALSE(skinned_mesh->HasRaytraceBvhCallback());
    EXPECT_EQ(skinned_mesh->GetBvhBufferId(), frame::NullId);
}

TEST_F(ParseLevelTest, RasterOptionKeepsCubemapOnlyLevelOnCubemapProgram)
{
    const auto asset_root = frame::file::FindDirectory("asset");
    const frame::json::LevelDataOptions options{
        .render_preset = frame::json::RenderPreset::Raster};
    const auto level_data = frame::json::ParseLevelData(
        glm::uvec2(320, 200),
        frame::file::FindFile("asset/json/cubemap.json"),
        asset_root,
        options);

    ASSERT_EQ(level_data.programs.size(), 1u);
    EXPECT_EQ(level_data.programs.front().name, "CubemapProgram");
    ASSERT_EQ(level_data.render_pass_programs.size(), 1u);
    EXPECT_EQ(
        level_data.render_pass_programs.front().render_time,
        frame::proto::NodeMesh::SKYBOX_RENDER_TIME);
    EXPECT_EQ(
        level_data.render_pass_programs.front().program_name, "CubemapProgram");
}

TEST_F(ParseLevelTest, RaytraceOptionKeepsCubemapOnlyLevelOnCubemapProgram)
{
    const auto asset_root = frame::file::FindDirectory("asset");
    const frame::json::LevelDataOptions options{
        .render_preset = frame::json::RenderPreset::Raytrace};
    const auto level_data = frame::json::ParseLevelData(
        glm::uvec2(320, 200),
        frame::file::FindFile("asset/json/cubemap.json"),
        asset_root,
        options);

    ASSERT_EQ(level_data.programs.size(), 1u);
    EXPECT_EQ(level_data.programs.front().name, "CubemapProgram");
    ASSERT_EQ(level_data.render_pass_programs.size(), 1u);
    EXPECT_EQ(
        level_data.render_pass_programs.front().render_time,
        frame::proto::NodeMesh::SKYBOX_RENDER_TIME);
    EXPECT_EQ(
        level_data.render_pass_programs.front().program_name, "CubemapProgram");

    auto built = frame::vulkan::BuildLevel(glm::uvec2(320, 200), level_data);
    ASSERT_NE(built.level, nullptr);
    const auto skybox_program_id = built.level->GetRenderPassProgramId(
        frame::proto::NodeMesh::SKYBOX_RENDER_TIME);
    ASSERT_NE(skybox_program_id, frame::NullId);
    EXPECT_EQ(built.level->GetNameFromId(skybox_program_id), "CubemapProgram");
    EXPECT_EQ(
        built.level->GetRenderPassProgramId(
            frame::proto::NodeMesh::SCENE_RENDER_TIME),
        frame::NullId);
}

} // End namespace test.
