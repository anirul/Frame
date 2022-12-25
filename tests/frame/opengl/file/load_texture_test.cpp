#include "frame/opengl/file/load_texture_test.h"

#include "frame/file/file_system.h"
#include "frame/opengl/file/load_texture.h"
#include "frame/opengl/texture.h"
#include <algorithm>

namespace test {

TEST_F(LoadTextureTest, LoadTextureFromFloatTest) {
    auto texture = frame::opengl::file::LoadTextureFromFloat(0.1f);
    EXPECT_TRUE(texture);
    EXPECT_EQ(1, texture->GetSize().x);
    EXPECT_EQ(1, texture->GetSize().y);
    auto vecf            = texture->GetTextureFloat();
    EXPECT_FLOAT_EQ(0.1f, *vecf.data());
}

TEST_F(LoadTextureTest, LoadTextureFromVec4Test) {
    auto texture = frame::opengl::file::LoadTextureFromVec4(glm::vec4(0.1f, 0.2f, 0.3f, 0.4f));
    EXPECT_TRUE(texture);
    EXPECT_EQ(1, texture->GetSize().x);
    EXPECT_EQ(1, texture->GetSize().y);
    auto vecf = texture->GetTextureFloat();
    EXPECT_EQ(4, vecf.size());
    EXPECT_FLOAT_EQ(0.1f, vecf[0]);
    EXPECT_FLOAT_EQ(0.2f, vecf[1]);
    EXPECT_FLOAT_EQ(0.3f, vecf[2]);
    EXPECT_FLOAT_EQ(0.4f, vecf[3]);
}

TEST_F(LoadTextureTest, LoadTextureFromFileTest) {
    auto texture = frame::opengl::file::LoadTextureFromFile(
        frame::file::FindFile("asset/cubemap/positive_x.png"),
        frame::proto::PixelElementSize_BYTE(), frame::proto::PixelStructure_RGB_ALPHA());
    EXPECT_TRUE(texture);
    EXPECT_EQ(1024, texture->GetSize().x);
    EXPECT_EQ(1024, texture->GetSize().y);
    auto vec8 = texture->GetTextureByte();
    auto it_pair = std::minmax_element(vec8.begin(), vec8.end());
    EXPECT_EQ(0x59, *it_pair.first);
    EXPECT_EQ(0xff, *it_pair.second);
}

// TODO(anirul): Add a test for the load Cubemap from single file when implemented.

TEST_F(LoadTextureTest, LoadCubeMapFromFilesTest) {
    auto texture = frame::opengl::file::LoadCubeMapTextureFromFiles(
        { frame::file::FindFile("asset/cubemap/positive_x.png"),
          frame::file::FindFile("asset/cubemap/negative_x.png"),
          frame::file::FindFile("asset/cubemap/positive_y.png"),
          frame::file::FindFile("asset/cubemap/negative_y.png"),
          frame::file::FindFile("asset/cubemap/positive_z.png"),
          frame::file::FindFile("asset/cubemap/negative_z.png") },
        frame::proto::PixelElementSize_BYTE(), frame::proto::PixelStructure_RGB_ALPHA());
    EXPECT_TRUE(texture);
    EXPECT_EQ(1024, texture->GetSize().x);
    EXPECT_EQ(1024, texture->GetSize().y);
    auto vec8     = texture->GetTextureByte();
    auto it_pair  = std::minmax_element(vec8.begin(), vec8.end());
    EXPECT_EQ(0x49, *it_pair.first);
    EXPECT_EQ(0xff, *it_pair.second);
}

}  // End namespace test.
