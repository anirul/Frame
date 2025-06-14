#include "frame/opengl/file/load_texture_test.h"

#include <algorithm>

#include "frame/file/file_system.h"
#include "frame/file/image.h"
#include "frame/json/parse_uniform.h"
#include "frame/opengl/cubemap.h"
#include "frame/opengl/file/load_texture.h"
#include "frame/opengl/texture.h"

namespace test
{

TEST_F(LoadTextureTest, LoadTextureFromFloatTest)
{
    auto texture = frame::opengl::file::LoadTextureFromFloat(0.1f);
    EXPECT_TRUE(texture);
    auto texture_size = frame::json::ParseSize(texture->GetData().size());
    EXPECT_EQ(1, texture_size.x);
    EXPECT_EQ(1, texture_size.y);
    auto vecf = texture->GetTextureFloat();
    EXPECT_FLOAT_EQ(0.1f, *vecf.data());
}

TEST_F(LoadTextureTest, LoadTextureFromVec4Test)
{
    auto texture = frame::opengl::file::LoadTextureFromVec4(
        glm::vec4(0.1f, 0.2f, 0.3f, 0.4f));
    EXPECT_TRUE(texture);
    auto texture_size = frame::json::ParseSize(texture->GetData().size());
    EXPECT_EQ(1, texture_size.x);
    EXPECT_EQ(1, texture_size.y);
    auto vecf = texture->GetTextureFloat();
    EXPECT_EQ(4, vecf.size());
    EXPECT_FLOAT_EQ(0.1f, vecf[0]);
    EXPECT_FLOAT_EQ(0.2f, vecf[1]);
    EXPECT_FLOAT_EQ(0.3f, vecf[2]);
    EXPECT_FLOAT_EQ(0.4f, vecf[3]);
}

TEST_F(LoadTextureTest, LoadTextureFromFileTest)
{
    auto texture = std::make_unique<frame::opengl::Texture>(
        frame::file::FindFile("asset/cubemap/positive_x.png"),
        frame::json::PixelElementSize_BYTE(),
        frame::json::PixelStructure_RGB_ALPHA());
    EXPECT_TRUE(texture);
    auto texture_size = frame::json::ParseSize(texture->GetData().size());
    EXPECT_EQ(1024, texture_size.x);
    EXPECT_EQ(1024, texture_size.y);
    auto vec8 = texture->GetTextureByte();
    auto it_pair = std::minmax_element(vec8.begin(), vec8.end());
    EXPECT_EQ(0x59, *it_pair.first);
    EXPECT_EQ(0xff, *it_pair.second);
}

TEST_F(LoadTextureTest, LoadHdrImageTest)
{
    frame::file::Image image(
        frame::file::FindFile("asset/cubemap/shiodome.hdr"),
        frame::json::PixelElementSize_FLOAT(),
        frame::json::PixelStructure_RGB());
    EXPECT_EQ(glm::uvec2(3200, 1600), image.GetSize());
    const float* ptr = static_cast<const float*>(image.Data());
    std::size_t len = image.GetSize().x * image.GetSize().y * 3;
    auto range = std::minmax_element(ptr, ptr + len);
    EXPECT_NEAR(0.0f, *range.first, 0.1f);
    EXPECT_GT(*range.second, 1.0f);
}

// TODO(anirul): Add a test for the load Cubemap from single file when
// implemented.

TEST_F(LoadTextureTest, LoadCubeMapFromFilesTest)
{
    auto texture = std::make_unique<frame::opengl::Cubemap>(
        std::array<std::filesystem::path, 6>{
            frame::file::FindFile("asset/cubemap/positive_x.png"),
            frame::file::FindFile("asset/cubemap/negative_x.png"),
            frame::file::FindFile("asset/cubemap/positive_y.png"),
            frame::file::FindFile("asset/cubemap/negative_y.png"),
            frame::file::FindFile("asset/cubemap/positive_z.png"),
            frame::file::FindFile("asset/cubemap/negative_z.png")},
        frame::json::PixelElementSize_BYTE(),
        frame::json::PixelStructure_RGB_ALPHA());
    EXPECT_TRUE(texture);
    auto texture_size = frame::json::ParseSize(texture->GetData().size());
    EXPECT_EQ(1024, texture_size.x);
    EXPECT_EQ(1024, texture_size.y);
    auto vec8 = texture->GetTextureByte();
    auto it_pair = std::minmax_element(vec8.begin(), vec8.end());
    EXPECT_EQ(0x49, *it_pair.first);
    EXPECT_EQ(0xff, *it_pair.second);
}

} // End namespace test.
