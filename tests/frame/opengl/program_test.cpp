#include "frame/opengl/program_test.h"

namespace test {

    TEST_F(ProgramTest, CreateProgramTest) {
        EXPECT_FALSE(program_);
        program_ = std::make_unique<frame::opengl::Program>("test");
        EXPECT_TRUE(program_);
    }

    TEST_F(ProgramTest, CheckShaderProgramTest) {
        EXPECT_FALSE(program_);
        program_ = std::make_unique<frame::opengl::Program>("test");
        EXPECT_TRUE(program_);
        auto program_ptr =
            dynamic_cast<frame::opengl::Program*>(program_.get());
        ASSERT_TRUE(program_ptr);
        frame::opengl::Shader vertex_shader(
            frame::opengl::ShaderEnum::VERTEX_SHADER);
        EXPECT_TRUE(vertex_shader.LoadFromSource(GetVertexSource()));
        program_ptr->AddShader(vertex_shader);
        frame::opengl::Shader fragment_shader(
            frame::opengl::ShaderEnum::FRAGMENT_SHADER);
        EXPECT_TRUE(fragment_shader.LoadFromSource(GetFragmentSource()));
        program_ptr->AddShader(fragment_shader);
    }

    TEST_F(ProgramTest, CheckUseAndLinkProgramTest) {
        EXPECT_FALSE(program_);
        program_ = std::make_unique<frame::opengl::Program>("test");
        auto program_ptr =
            dynamic_cast<frame::opengl::Program*>(program_.get());
        EXPECT_TRUE(program_);
        EXPECT_TRUE(program_ptr);
        frame::opengl::Shader vertex_shader(
            frame::opengl::ShaderEnum::VERTEX_SHADER);
        EXPECT_TRUE(vertex_shader.LoadFromSource(GetVertexSource()));
        program_ptr->AddShader(vertex_shader);
        frame::opengl::Shader fragment_shader(
            frame::opengl::ShaderEnum::FRAGMENT_SHADER);
        EXPECT_TRUE(fragment_shader.LoadFromSource(GetFragmentSource()));
        program_ptr->AddShader(fragment_shader);
        program_->LinkShader();
        program_->Use();
    }

    TEST_F(ProgramTest, CreateSimpleProgramProgramTest) {
        EXPECT_FALSE(program_);
        std::istringstream iss_vertex(GetVertexSource());
        std::istringstream iss_fragment(GetFragmentSource());
        auto program =
            frame::opengl::CreateProgram("test", iss_vertex, iss_fragment);
        ASSERT_TRUE(program);
    }

    TEST_F(ProgramTest, UniformTest) {
        EXPECT_FALSE(program_);
        std::istringstream iss_vertex(GetVertexSource());
        std::istringstream iss_fragment(GetFragmentSource());
        auto program =
            frame::opengl::CreateProgram("test", iss_vertex, iss_fragment);
        ASSERT_TRUE(program);
        program_ = std::move(program);
        auto uniform_list = program_->GetUniformNameList();
        ASSERT_EQ(4, uniform_list.size());
        ASSERT_EQ(1,
            std::count(uniform_list.begin(), uniform_list.end(), "projection"));
        ASSERT_EQ(
            1,
            std::count(uniform_list.begin(), uniform_list.end(), "view"));
        ASSERT_EQ(
            1,
            std::count(uniform_list.begin(), uniform_list.end(), "model"));
        ASSERT_EQ(
            1,
            std::count(uniform_list.begin(), uniform_list.end(), "Color"));
        EXPECT_TRUE(program_);
    }

    const std::string ProgramTest::GetVertexSource() const {
        return R"vert(
#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

out vec3 vert_normal;
out vec3 vert_position;
out vec2 vert_texcoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
	vert_normal = normalize(vec3(model * vec4(in_normal, 1.0)));
	vert_texcoord = in_texcoord;
	mat4 pvm = projection * view * model;
	vert_position = (pvm * vec4(in_position, 1.0)).xyz;
	gl_Position = pvm * vec4(in_position, 1.0);
}
		)vert";
    }

    const std::string ProgramTest::GetFragmentSource() const {
        return R"frag(
#version 330 core

in vec3 vert_normal;
in vec2 vert_texcoord;
in vec3 vert_position;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_zbuffer;

uniform sampler2D Color;

const vec3 light_position = vec3(-1.0, 1.0, 1.0);

void main()
{
	float shade = clamp(dot(light_position, vert_normal), 0, 1);
	vec3 color = vec3(texture(Color, vert_texcoord));
	frag_color = vec4(shade * color, 1.0);
	// Create a Z buffer value.
	float z_value = vert_position.z / 8;
	frag_zbuffer = vec4(z_value, z_value, z_value, 1.0);
}
		)frag";
    }

}  // End namespace test.
