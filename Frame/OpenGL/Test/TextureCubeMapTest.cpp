#include "TextureCubeMapTest.h"

#include <GL/glew.h>
#include "Frame/File/FileSystem.h"
#include "Frame/OpenGL/File/LoadTexture.h"
#include "Frame/Proto/ParseTexture.h"

namespace test {

    TEST_F(TextureCubeMapTest, CreateTextureCubeMapTest)
    {
        EXPECT_EQ(GLEW_OK, glewInit());
        ASSERT_FALSE(texture_);
        std::optional<std::unique_ptr<frame::TextureInterface>> maybe_texture;
        EXPECT_NO_THROW(
            maybe_texture = frame::opengl::file::LoadCubeMapTextureFromFiles(
                std::array<std::string, 6>{
                    frame::file::FindFile("Asset/CubeMap/PositiveX.png"),
                    frame::file::FindFile("Asset/CubeMap/NegativeX.png"),
                    frame::file::FindFile("Asset/CubeMap/PositiveY.png"),
                    frame::file::FindFile("Asset/CubeMap/NegativeY.png"),
                    frame::file::FindFile("Asset/CubeMap/PositiveZ.png"),
                    frame::file::FindFile("Asset/CubeMap/NegativeZ.png")
                }));
        ASSERT_TRUE(maybe_texture);
        texture_ = std::move(maybe_texture.value());
        ASSERT_TRUE(texture_);
        ASSERT_NE(0, texture_->GetId());
    }

    TEST_F(TextureCubeMapTest, CreateEquirectangularTextureCubeMapTest)
    {
        EXPECT_EQ(GLEW_OK, glewInit());
        ASSERT_FALSE(texture_);
        std::optional<std::unique_ptr<frame::TextureInterface>> maybe_texture;
        EXPECT_NO_THROW(
            maybe_texture = frame::opengl::file::LoadCubeMapTextureFromFile(
                frame::file::FindFile("Asset/CubeMap/Hamarikyu.hdr"),
                frame::proto::PixelElementSize_HALF()));
        ASSERT_TRUE(maybe_texture);
        texture_ = std::move(maybe_texture.value());
        ASSERT_TRUE(texture_);
        EXPECT_NE(0, texture_->GetId());
    }

} // End namespace test.
