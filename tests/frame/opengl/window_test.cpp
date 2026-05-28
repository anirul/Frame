#include "frame/opengl/window_test.h"

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"
#include "frame/level_data_startup_internal.h"
#include "frame/opengl/window_factory.h"

namespace test
{

TEST_F(WindowTest, CreateWindowTest)
{
    EXPECT_FALSE(window_);
    window_ = frame::opengl::CreateSDLOpenGLNone({640, 512});
    EXPECT_TRUE(window_);
}

TEST_F(WindowTest, GetSizeWindowTest)
{
    ASSERT_FALSE(window_);
    window_ = frame::opengl::CreateSDLOpenGLNone({640, 512});
    ASSERT_TRUE(window_);
    glm::uvec2 pair = {640, 512};
    EXPECT_EQ(pair, window_->GetSize());
}

TEST_F(WindowTest, CreateDeviceWindowTest)
{
    ASSERT_FALSE(window_);
    window_ = frame::opengl::CreateSDLOpenGLNone({640, 512});
    ASSERT_TRUE(window_);
    EXPECT_EQ(window_->GetDrawingTargetEnum(), frame::DrawingTargetEnum::NONE);
    EXPECT_EQ(
        window_->GetDevice().GetDeviceEnum(), frame::RenderingAPIEnum::OPENGL);
}

TEST_F(WindowTest, DestroyWindowAfterLoadedRasterLevelKeepsContextAlive)
{
    ASSERT_FALSE(window_);
    window_ = frame::opengl::CreateSDLOpenGLNone({320, 200});
    ASSERT_TRUE(window_);

    const auto asset_root = frame::file::FindDirectory("asset");
    const frame::json::LevelDataOptions options{
        .render_preset = frame::json::RenderPreset::Raster};
    const auto level_data = frame::json::ParseLevelData(
        glm::uvec2(320, 200),
        frame::file::FindFile("asset/json/raytracing.json"),
        asset_root,
        options);

    auto* loader = dynamic_cast<frame::internal::LevelDataStartupInterface*>(
        &window_->GetDevice());
    ASSERT_NE(loader, nullptr);
    ASSERT_NO_THROW(loader->StartupFromLevelData(level_data));
    EXPECT_NO_THROW(window_.reset());
}

} // End namespace test.
