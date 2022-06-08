#include "frame/open_gl/pixel_test.h"

#include <GL/glew.h>

#include "frame/json/parse_pixel.h"

namespace test {

TEST_F(PixelTest, ConvertToGLTypePixelElementPixelTest) {
    EXPECT_EQ(GL_UNSIGNED_BYTE,
              frame::opengl::ConvertToGLType(frame::proto::PixelElementSize_BYTE()));
    EXPECT_EQ(GL_UNSIGNED_SHORT,
              frame::opengl::ConvertToGLType(frame::proto::PixelElementSize_SHORT()));
    EXPECT_EQ(GL_FLOAT, frame::opengl::ConvertToGLType(frame::proto::PixelElementSize_HALF()));
    EXPECT_EQ(GL_FLOAT, frame::opengl::ConvertToGLType(frame::proto::PixelElementSize_FLOAT()));
}

TEST_F(PixelTest, ConvertToGLTypePixelStructurePixelTest) {
    EXPECT_EQ(GL_RED, frame::opengl::ConvertToGLType(frame::proto::PixelStructure_GREY()));
    EXPECT_EQ(GL_RG, frame::opengl::ConvertToGLType(frame::proto::PixelStructure_GREY_ALPHA()));
    EXPECT_EQ(GL_RGB, frame::opengl::ConvertToGLType(frame::proto::PixelStructure_RGB()));
    EXPECT_EQ(GL_RGBA, frame::opengl::ConvertToGLType(frame::proto::PixelStructure_RGB_ALPHA()));
}

TEST_F(PixelTest, ConvertToGLTypePixelElementAndStructurePixelTest) {
    EXPECT_EQ(GL_R8, frame::opengl::ConvertToGLType(frame::proto::PixelElementSize_BYTE(),
                                                    frame::proto::PixelStructure_GREY()));
    EXPECT_EQ(GL_RG8, frame::opengl::ConvertToGLType(frame::proto::PixelElementSize_BYTE(),
                                                     frame::proto::PixelStructure_GREY_ALPHA()));
    EXPECT_EQ(GL_RGB8, frame::opengl::ConvertToGLType(frame::proto::PixelElementSize_BYTE(),
                                                      frame::proto::PixelStructure_RGB()));
    EXPECT_EQ(GL_RGBA8, frame::opengl::ConvertToGLType(frame::proto::PixelElementSize_BYTE(),
                                                       frame::proto::PixelStructure_RGB_ALPHA()));

    EXPECT_EQ(GL_R16, frame::opengl::ConvertToGLType(frame::proto::PixelElementSize_SHORT(),
                                                     frame::proto::PixelStructure_GREY()));
    EXPECT_EQ(GL_RG16, frame::opengl::ConvertToGLType(frame::proto::PixelElementSize_SHORT(),
                                                      frame::proto::PixelStructure_GREY_ALPHA()));
    EXPECT_EQ(GL_RGB16, frame::opengl::ConvertToGLType(frame::proto::PixelElementSize_SHORT(),
                                                       frame::proto::PixelStructure_RGB()));
    EXPECT_EQ(GL_RGBA16, frame::opengl::ConvertToGLType(frame::proto::PixelElementSize_SHORT(),
                                                        frame::proto::PixelStructure_RGB_ALPHA()));

    EXPECT_EQ(GL_R32F, frame::opengl::ConvertToGLType(frame::proto::PixelElementSize_FLOAT(),
                                                      frame::proto::PixelStructure_GREY()));
    EXPECT_EQ(GL_RG32F, frame::opengl::ConvertToGLType(frame::proto::PixelElementSize_FLOAT(),
                                                       frame::proto::PixelStructure_GREY_ALPHA()));
    EXPECT_EQ(GL_RGB32F, frame::opengl::ConvertToGLType(frame::proto::PixelElementSize_FLOAT(),
                                                        frame::proto::PixelStructure_RGB()));
    EXPECT_EQ(GL_RGBA32F, frame::opengl::ConvertToGLType(frame::proto::PixelElementSize_FLOAT(),
                                                         frame::proto::PixelStructure_RGB_ALPHA()));
}

}  // End namespace test.
