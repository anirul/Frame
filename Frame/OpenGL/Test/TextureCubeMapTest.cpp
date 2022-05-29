#include "TextureCubeMapTest.h"

#include <algorithm>
#include <GL/glew.h>
#include "Frame/File/FileSystem.h"
#include "Frame/OpenGL/File/LoadTexture.h"
#include "Frame/Proto/ParseTexture.h"

namespace test {

    TEST_F(TextureCubeMapTest, CreateTextureCubeMapTest)
    {
        ASSERT_FALSE(texture_);
        EXPECT_NO_THROW(
            texture_ = frame::opengl::file::LoadCubeMapTextureFromFiles(
                std::array<std::string, 6>{
                    frame::file::FindFile("Asset/CubeMap/PositiveX.png"),
                    frame::file::FindFile("Asset/CubeMap/NegativeX.png"),
                    frame::file::FindFile("Asset/CubeMap/PositiveY.png"),
                    frame::file::FindFile("Asset/CubeMap/NegativeY.png"),
                    frame::file::FindFile("Asset/CubeMap/PositiveZ.png"),
                    frame::file::FindFile("Asset/CubeMap/NegativeZ.png")
                }));
        ASSERT_TRUE(texture_);
        ASSERT_NE(0, texture_->GetId());
        EXPECT_EQ(1024, texture_->GetSize().first);
        EXPECT_EQ(1024, texture_->GetSize().second);
        auto vec8 = texture_->GetTextureByte();
        auto p = std::minmax_element(vec8.begin(), vec8.end());
        EXPECT_EQ(0x49, *p.first);
        EXPECT_EQ(0xff, *p.second);
    }

    TEST_F(TextureCubeMapTest, DISABLED_CreateEquirectangularTextureCubeMapTest)
    {
        ASSERT_FALSE(texture_);
        EXPECT_NO_THROW(
            texture_ = frame::opengl::file::LoadCubeMapTextureFromFile(
                frame::file::FindFile("Asset/CubeMap/Hamarikyu.hdr"),
                frame::proto::PixelElementSize_FLOAT()));
        ASSERT_TRUE(texture_);
        EXPECT_NE(0, texture_->GetId());
        EXPECT_EQ(512, texture_->GetSize().first);
        EXPECT_EQ(512, texture_->GetSize().second);
        auto vecf = texture_->GetTextureFloat();
        // Image size time the number of color per pixel time 6 (cubemap).
        EXPECT_EQ(512 * 512 * 3 * 6, vecf.size());
        auto p = std::minmax_element(vecf.begin(), vecf.end());
        // Should probably make a GT and LT...
        EXPECT_EQ(0.0f, *p.first);
        EXPECT_FLOAT_EQ(20.413086f, *p.second);
    }

} // End namespace test.
