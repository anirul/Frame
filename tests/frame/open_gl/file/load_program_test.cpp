#include "frame/open_gl/file/load_program_test.h"

#include "frame/open_gl/file/load_program.h"

namespace test {

TEST_F(LoadProgramTest, LoadFromNameTest) { ASSERT_TRUE(frame::opengl::file::LoadProgram("blur")); }

TEST_F(LoadProgramTest, LoadFromFileTest) {
    ASSERT_TRUE(frame::opengl::file::LoadProgram("asset/shader/open_gl/blur.vert",
                                                 "asset/shader/open_gl/blur.frag"));
}

}  // End namespace test.
