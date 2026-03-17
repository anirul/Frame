#include "frame/json/parse_program_test.h"

#include <algorithm>

#include "frame/opengl/json/parse_program.h"

namespace test
{

namespace
{

frame::proto::Program MakeBlurProgram()
{
    frame::proto::Program proto_program;
    proto_program.set_name("BlurProgram");
    proto_program.add_input_texture_names("Image");
    proto_program.add_output_texture_names("output");
    proto_program.mutable_input_scene_type()->set_value(
        frame::proto::SceneType::SCENE);
    proto_program.set_input_scene_root_name("root");
    auto* exponent = proto_program.add_uniforms();
    exponent->set_name("exponent");
    exponent->set_uniform_float(1.0f);
    return proto_program;
}

frame::json::ShaderFiles GetBlurShaderFiles()
{
    return {
        .vertex_shader = "blur.vert",
        .fragment_shader = "blur.frag"};
}

} // namespace

TEST_F(ParseProgramTest, CreateParseProgramTest)
{
    const auto proto_program = MakeBlurProgram();
    auto program = frame::json::ParseProgramOpenGL(
        proto_program,
        GetBlurShaderFiles(),
        *level_);
    EXPECT_TRUE(program);
}

TEST_F(ParseProgramTest, CreateParseProgramUniformTest)
{
    const auto proto_program = MakeBlurProgram();
    auto program = frame::json::ParseProgramOpenGL(
        proto_program,
        GetBlurShaderFiles(),
        *level_);
    EXPECT_TRUE(program);
    program_ = std::move(program);
    const auto uniform_list = program_->GetUniformNameList();
    EXPECT_EQ(2, uniform_list.size());
    EXPECT_EQ(
        1, std::count(uniform_list.begin(), uniform_list.end(), "exponent"));
    EXPECT_TRUE(program_);
}

} // End namespace test.
