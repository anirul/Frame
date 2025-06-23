#include "frame/opengl/file/load_program_test.h"

#include "frame/opengl/file/load_program.h"

namespace test
{

TEST_F(LoadProgramTest, LoadFromNameTest)
{
    frame::proto::Program proto_program;
    proto_program.set_name("blur");
    proto_program.set_shader_vertex("blur.vert");
    proto_program.set_shader_fragment("blur.frag");
    ASSERT_TRUE(frame::opengl::file::LoadProgram(proto_program));
}

TEST_F(LoadProgramTest, LoadFromFileTest)
{
    frame::proto::Program proto_program;
    proto_program.set_name("blur");
    proto_program.set_shader_vertex("blur.vert");
    proto_program.set_shader_fragment("blur.frag");
    ASSERT_TRUE(
        frame::opengl::file::LoadProgram(
            proto_program,
            "asset/shader/opengl/blur.vert",
            "asset/shader/opengl/blur.frag"));
}

} // End namespace test.
