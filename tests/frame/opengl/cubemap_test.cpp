#include "frame/opengl/cubemap_test.h"

#include <GL/glew.h>

#include <algorithm>

#include "frame/file/file_system.h"
#include "frame/json/parse_texture.h"
#include "frame/opengl/file/load_texture.h"
#include "frame/opengl/cubemap.h"

namespace test
{

TEST_F(CubemapTest, CreateCubemapTest)
{
    ASSERT_FALSE(texture_);
    EXPECT_NO_THROW(
        texture_ = std::make_unique<frame::opengl::Cubemap>(
            std::array<std::filesystem::path, 6>{
                frame::file::FindFile("asset/cubemap/positive_x.png"),
                frame::file::FindFile("asset/cubemap/negative_x.png"),
                frame::file::FindFile("asset/cubemap/positive_y.png"),
                frame::file::FindFile("asset/cubemap/negative_y.png"),
                frame::file::FindFile("asset/cubemap/positive_z.png"),
                frame::file::FindFile("asset/cubemap/negative_z.png")}));
    ASSERT_TRUE(texture_);
    auto* opengl_texture =
        dynamic_cast<frame::opengl::Cubemap*>(texture_.get());
    ASSERT_NE(nullptr, opengl_texture);
    ASSERT_NE(0, opengl_texture->GetId());
    EXPECT_EQ(1024, texture_->GetData().size().x());
    EXPECT_EQ(1024, texture_->GetData().size().y());
    auto vec8 = texture_->GetTextureByte();
    EXPECT_EQ(1024 * 1024 * 3 * 6, vec8.size());
    auto p = std::minmax_element(vec8.begin(), vec8.end());
    EXPECT_EQ(0x49, *p.first);
    EXPECT_EQ(0xff, *p.second);
}

TEST_F(CubemapTest, CreateEquirectangularCubemapTest)
{
    ASSERT_FALSE(texture_);
    EXPECT_NO_THROW(
        texture_ = std::make_unique<frame::opengl::Cubemap>(
            frame::file::FindFile("asset/cubemap/hamarikyu.hdr"),
            frame::json::PixelElementSize_FLOAT()));
    ASSERT_TRUE(texture_);
    auto* opengl_texture =
        dynamic_cast<frame::opengl::Cubemap*>(texture_.get());
    ASSERT_NE(nullptr, opengl_texture);
    ASSERT_NE(0, opengl_texture->GetId());
    EXPECT_EQ(1024, texture_->GetData().size().x());
    EXPECT_EQ(1024, texture_->GetData().size().y());
    auto vecf = texture_->GetTextureFloat();
    // Image size time the number of color per pixel time 6 (cubemap).
    EXPECT_EQ(1024 * 1024 * 3 * 6, vecf.size());
    auto p = std::minmax_element(vecf.begin(), vecf.end());
    // Use a EXPECT_NEAR as using equality had tendencies to fail.
    EXPECT_NEAR(0.0f, *p.first, 0.1f);
    EXPECT_NEAR(20.0f, *p.second, 1.0f);
}

} // End namespace test.
