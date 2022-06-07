#include "frame/open_gl/file/load_program_test.h"

#include "frame/open_gl/file/load_program.h"

namespace test {

TEST_F(LoadProgramTest, LoadFromNameTest) { ASSERT_TRUE(frame::opengl::file::LoadProgram("Blur")); }

TEST_F(LoadProgramTest, LoadFromFileTest) {
    ASSERT_TRUE(frame::opengl::file::LoadProgram("Asset/Shader/OpenGL/Blur.vert",
                                                 "Asset/Shader/OpenGL/Blur.frag"));
}

}  // End namespace test.
