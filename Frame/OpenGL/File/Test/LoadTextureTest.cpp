#include "LoadTextureTest.h"

#include "Frame/File/FileSystem.h"
#include "Frame/OpenGL/File/LoadTexture.h"
#include "Frame/OpenGL/Texture.h"

namespace test {

TEST_F(LoadTextureTest, LoadTextureFromFloatTest) {
    auto texture = frame::opengl::file::LoadTextureFromFloat(0.1f);
    EXPECT_TRUE(texture);
    EXPECT_EQ(1, texture->GetSize().first);
    EXPECT_EQ(1, texture->GetSize().second);
    auto* opengl_texture = dynamic_cast<frame::opengl::Texture*>(texture.get());
    opengl_texture->Bind();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    float f = 0.0f;
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, &f);
    EXPECT_FLOAT_EQ(0.1f, f);
    opengl_texture->UnBind();
}

TEST_F(LoadTextureTest, LoadTextureFromVec4Test) {
    auto texture = frame::opengl::file::LoadTextureFromVec4(glm::vec4(0.1f, 0.2f, 0.3f, 0.4f));
    EXPECT_TRUE(texture);
    EXPECT_EQ(1, texture->GetSize().first);
    EXPECT_EQ(1, texture->GetSize().second);
    auto vecf = texture->GetTextureFloat();
    EXPECT_EQ(4, vecf.size());
    EXPECT_FLOAT_EQ(0.1f, vecf[0]);
    EXPECT_FLOAT_EQ(0.2f, vecf[1]);
    EXPECT_FLOAT_EQ(0.3f, vecf[2]);
    EXPECT_FLOAT_EQ(0.4f, vecf[3]);
}

TEST_F(LoadTextureTest, LoadTextureFromFileTest) {
    auto texture = frame::opengl::file::LoadTextureFromFile(
        frame::file::FindFile("Asset/CubeMap/PositiveX.png"), frame::proto::PixelElementSize_BYTE(),
        frame::proto::PixelStructure_RGB_ALPHA());
    EXPECT_TRUE(texture);
    EXPECT_EQ(1024, texture->GetSize().first);
    EXPECT_EQ(1024, texture->GetSize().second);
    auto vec8 = texture->GetTextureByte();
    for (int position = 0; position < vec8.size(); position += 1025 * 2) {
        EXPECT_GE(0xff, vec8[position]);
        EXPECT_LE(0x4e, vec8[position]);
    }
}

// TODO(anirul): Add a test for the load Cubemap from single file when implemented.

TEST_F(LoadTextureTest, LoadCubeMapFromFilesTest) {
    auto texture = frame::opengl::file::LoadCubeMapTextureFromFiles(
        { frame::file::FindFile("Asset/CubeMap/PositiveX.png"),
          frame::file::FindFile("Asset/CubeMap/NegativeX.png"),
          frame::file::FindFile("Asset/CubeMap/PositiveY.png"),
          frame::file::FindFile("Asset/CubeMap/NegativeY.png"),
          frame::file::FindFile("Asset/CubeMap/PositiveZ.png"),
          frame::file::FindFile("Asset/CubeMap/NegativeZ.png") },
        frame::proto::PixelElementSize_BYTE(), frame::proto::PixelStructure_RGB_ALPHA());
    EXPECT_TRUE(texture);
    EXPECT_EQ(1024, texture->GetSize().first);
    EXPECT_EQ(1024, texture->GetSize().second);
    auto vec8     = texture->GetTextureByte();
    bool not_full = false;
    for (int position = 0; position < vec8.size(); position += 1025 * 5) {
        if (vec8[position] != 0xff) not_full = true;
        EXPECT_GE(0xff, vec8[position]);
        EXPECT_LE(0x45, vec8[position]);
    }
    EXPECT_TRUE(not_full);
}

}  // End namespace test.
