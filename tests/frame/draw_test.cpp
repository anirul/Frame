#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "absl/flags/flag.h"

#include "frame/common/application.h"
#include "frame/common/draw.h"
#include "frame/device_interface.h"
#include "frame/file/file_system.h"
#include "frame/json/program_key.h"
#include "frame/level_data_startup_internal.h"

namespace test
{
namespace
{

class LevelDataStartupDevice final
    : public frame::DeviceInterface,
      public frame::internal::LevelDataStartupInterface
{
  public:
    void StartupFromLevelData(const frame::json::LevelData& level_data) override
    {
        ++startup_count;
        captured_level_data = level_data;
    }

    void Clear(const glm::vec4&) const override
    {
    }

    void Startup(std::unique_ptr<frame::LevelInterface>&& level) override
    {
        level_ = std::move(level);
    }

    void AddPlugin(std::unique_ptr<frame::PluginInterface>&&) override
    {
    }

    std::vector<frame::PluginInterface*> GetPluginPtrs() override
    {
        return {};
    }

    std::vector<std::string> GetPluginNames() const override
    {
        return {};
    }

    void RemovePluginByName(const std::string&) override
    {
    }

    void Display(double) override
    {
    }

    void Cleanup() override
    {
    }

    void Resize(glm::uvec2 size) override
    {
        size_ = size;
    }

    glm::uvec2 GetSize() const override
    {
        return size_;
    }

    frame::LevelInterface& GetLevel() override
    {
        if (!level_)
        {
            throw std::runtime_error("No level loaded.");
        }
        return *level_;
    }

    void* GetDeviceContext() const override
    {
        return nullptr;
    }

    void ScreenShot(const std::string&) const override
    {
    }

    void SetStereo(
        frame::StereoEnum stereo_enum,
        float interocular_distance,
        glm::vec3 focus_point = glm::vec3(0.0f),
        bool invert_left_right = false) override
    {
        stereo_enum_ = stereo_enum;
        interocular_distance_ = interocular_distance;
        focus_point_ = focus_point;
        invert_left_right_ = invert_left_right;
    }

    frame::RenderingAPIEnum GetDeviceEnum() const override
    {
        return frame::RenderingAPIEnum::OPENGL;
    }

    frame::StereoEnum GetStereoEnum() const override
    {
        return stereo_enum_;
    }

    float GetInteroccularDistance() const override
    {
        return interocular_distance_;
    }

    glm::vec3 GetFocusPoint() const override
    {
        return focus_point_;
    }

    std::unique_ptr<frame::BufferInterface> CreatePointBuffer(
        std::vector<float>&&) override
    {
        return nullptr;
    }

    std::unique_ptr<frame::BufferInterface> CreateIndexBuffer(
        std::vector<std::uint32_t>&&) override
    {
        return nullptr;
    }

    std::unique_ptr<frame::MeshInterface> CreateMesh(
        const frame::MeshParameter&) override
    {
        return nullptr;
    }

    int startup_count = 0;
    std::optional<frame::json::LevelData> captured_level_data = std::nullopt;

  private:
    glm::uvec2 size_ = glm::uvec2(320, 200);
    frame::StereoEnum stereo_enum_ = frame::StereoEnum::NONE;
    float interocular_distance_ = 0.0f;
    glm::vec3 focus_point_ = glm::vec3(0.0f);
    bool invert_left_right_ = false;
    std::unique_ptr<frame::LevelInterface> level_ = nullptr;
};

class RenderingFlagGuard
{
  public:
    RenderingFlagGuard() : previous_value_(absl::GetFlag(FLAGS_rendering))
    {
    }

    ~RenderingFlagGuard()
    {
        absl::SetFlag(&FLAGS_rendering, previous_value_);
    }

  private:
    std::string previous_value_;
};

frame::json::LevelData StartupDrawWithRenderingFlag(const std::string& flag)
{
    absl::SetFlag(&FLAGS_rendering, flag);

    LevelDataStartupDevice device;
    frame::common::Draw draw(
        glm::uvec2(320, 200),
        frame::file::FindFile("asset/json/raytracing.json"),
        device);
    draw.Startup(glm::uvec2(320, 200));

    EXPECT_EQ(device.startup_count, 1);
    EXPECT_TRUE(device.captured_level_data.has_value());
    return *device.captured_level_data;
}

bool HasNode(const frame::json::LevelData& level_data, const std::string& name)
{
    return std::find_if(
               level_data.proto.scene_tree().node_meshes().begin(),
               level_data.proto.scene_tree().node_meshes().end(),
               [&](const frame::proto::NodeMesh& node) {
                   return node.name() == name;
               }) != level_data.proto.scene_tree().node_meshes().end();
}

std::optional<frame::json::RenderPassProgramInfo> FindRenderPass(
    const frame::json::LevelData& level_data,
    frame::proto::NodeMesh::RenderTimeEnum render_time)
{
    const auto it = std::find_if(
        level_data.render_pass_programs.begin(),
        level_data.render_pass_programs.end(),
        [&](const frame::json::RenderPassProgramInfo& pass) {
            return pass.render_time == render_time;
        });
    if (it == level_data.render_pass_programs.end())
    {
        return std::nullopt;
    }
    return *it;
}

bool HasRaytracingProgram(const frame::json::LevelData& level_data)
{
    return std::any_of(
        level_data.programs.begin(),
        level_data.programs.end(),
        [](const frame::json::ProgramInfo& program) {
            return frame::json::IsRaytracingProgramKey(
                frame::json::ResolveProgramKey(program.proto));
        });
}

} // namespace

TEST(DrawRenderingOverrideTest, RasterAliasesStartRasterLevelData)
{
    RenderingFlagGuard flag_guard;

    const std::vector<std::string> aliases = {
        "raster", "rasterise", "rasterising", "rasterize", "rasterizing"};
    for (const auto& alias : aliases)
    {
        const auto level_data = StartupDrawWithRenderingFlag(alias);

        EXPECT_FALSE(HasNode(level_data, "RayTracingRendering")) << alias;
        EXPECT_FALSE(HasRaytracingProgram(level_data)) << alias;
        const auto scene_pass = FindRenderPass(
            level_data, frame::proto::NodeMesh::SCENE_RENDER_TIME);
        ASSERT_TRUE(scene_pass.has_value()) << alias;
        EXPECT_EQ(scene_pass->program_name, "RasterSceneProgram") << alias;
        EXPECT_TRUE(scene_pass->preprocess_program_name.empty()) << alias;
    }
}

TEST(DrawRenderingOverrideTest, RaytraceAliasesStartRaytraceLevelData)
{
    RenderingFlagGuard flag_guard;

    const std::vector<std::string> aliases = {"raytrace", "raytracing"};
    for (const auto& alias : aliases)
    {
        const auto level_data = StartupDrawWithRenderingFlag(alias);

        EXPECT_TRUE(HasNode(level_data, "RayTracingRendering")) << alias;
        EXPECT_TRUE(HasRaytracingProgram(level_data)) << alias;
        const auto scene_pass = FindRenderPass(
            level_data, frame::proto::NodeMesh::SCENE_RENDER_TIME);
        ASSERT_TRUE(scene_pass.has_value()) << alias;
        EXPECT_EQ(scene_pass->program_name, "RayTraceProgram") << alias;
        const auto preprocess_pass =
            FindRenderPass(level_data, frame::proto::NodeMesh::PRE_RENDER_TIME);
        ASSERT_TRUE(preprocess_pass.has_value()) << alias;
        EXPECT_EQ(preprocess_pass->program_name, "RayTraceProgram") << alias;
        EXPECT_EQ(
            preprocess_pass->preprocess_program_name,
            "RayTracePreprocessProgram")
            << alias;
    }
}

TEST(DrawRenderingOverrideTest, UnknownRenderingFlagThrows)
{
    RenderingFlagGuard flag_guard;
    absl::SetFlag(&FLAGS_rendering, "scanline");

    LevelDataStartupDevice device;
    frame::common::Draw draw(
        glm::uvec2(320, 200),
        frame::file::FindFile("asset/json/raytracing.json"),
        device);

    EXPECT_THROW(draw.Startup(glm::uvec2(320, 200)), std::invalid_argument);
    EXPECT_EQ(device.startup_count, 0);
}

} // namespace test
