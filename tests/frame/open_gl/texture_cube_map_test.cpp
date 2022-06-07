#include "frame/open_gl/texture_cube_map_test.h"

#include <GL/glew.h>

#include <algorithm>

#include "frame/file/file_system.h"
#include "frame/open_gl/file/load_texture.h"
#include "frame/open_gl/texture_cube_map.h"
#include "frame/proto/parse_texture.h"

namespace test {

TEST_F(TextureCubeMapTest, CreateTextureCubeMapTest) {
    ASSERT_FALSE(texture_);
    EXPECT_NO_THROW(
        texture_ = frame::opengl::file::LoadCubeMapTextureFromFiles(
            std::array<std::string, 6>{ frame::file::FindFile("asset/cubemap/positive_x.png"),
                                        frame::file::FindFile("asset/cubemap/negative_x.png"),
                                        frame::file::FindFile("asset/cubemap/positive_y.png"),
                                        frame::file::FindFile("asset/cubemap/negative_y.png"),
                                        frame::file::FindFile("asset/cubemap/positive_z.png"),
                                        frame::file::FindFile("asset/cubemap/negative_z.png") }));
    ASSERT_TRUE(texture_);
    auto* opengl_texture = dynamic_cast<frame::opengl::TextureCubeMap*>(texture_.get());
    ASSERT_NE(nullptr, opengl_texture);
    ASSERT_NE(0, opengl_texture->GetId());
    EXPECT_EQ(1024, texture_->GetSize().first);
    EXPECT_EQ(1024, texture_->GetSize().second);
    auto vec8 = texture_->GetTextureByte();
    EXPECT_EQ(1024 * 1024 * 3 * 6, vec8.size());
    auto p = std::minmax_element(vec8.begin(), vec8.end());
    EXPECT_EQ(0x49, *p.first);
    EXPECT_EQ(0xff, *p.second);
}

// FIXME(anirul): There is a problem in there... Note that the issue is slightly different with card
// and if you execute it stand alone. Is it short and not float?
TEST_F(TextureCubeMapTest, DISABLED_CreateEquirectangularTextureCubeMapTest) {
    ASSERT_FALSE(texture_);
    EXPECT_NO_THROW(texture_ = frame::opengl::file::LoadCubeMapTextureFromFile(
                        frame::file::FindFile("asset/cubemap/hamarikyu.hdr"),
                        frame::proto::PixelElementSize_FLOAT()));
    ASSERT_TRUE(texture_);
    auto* opengl_texture = dynamic_cast<frame::opengl::TextureCubeMap*>(texture_.get());
    ASSERT_NE(nullptr, opengl_texture);
    ASSERT_NE(0, opengl_texture->GetId());
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

}  // End namespace test.
