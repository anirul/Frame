#include "frame/opengl/file/load_program_test.h"

#include "frame/opengl/file/load_program.h"

namespace test {

TEST_F(LoadProgramTest, LoadFromNameTest) { ASSERT_TRUE(frame::opengl::file::LoadProgram("blur")); }

TEST_F(LoadProgramTest, LoadFromFileTest) {
    ASSERT_TRUE(frame::opengl::file::LoadProgram("blur", "asset/shader/opengl/blur.vert",
                                                 "asset/shader/opengl/blur.frag"));
}

}  // End namespace test.
