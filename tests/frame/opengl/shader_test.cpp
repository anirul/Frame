#include "frame/opengl/shader_test.h"

namespace test
{

TEST_F(ShaderTest, CreateVertexShaderTest)
{
    EXPECT_FALSE(shader_);
    shader_ = std::make_unique<frame::opengl::Shader>(
        frame::opengl::ShaderEnum::VERTEX_SHADER);
    EXPECT_TRUE(shader_);
}

TEST_F(ShaderTest, CreateFragmentShaderTest)
{
    EXPECT_FALSE(shader_);
    shader_ = std::make_unique<frame::opengl::Shader>(
        frame::opengl::ShaderEnum::FRAGMENT_SHADER);
    EXPECT_TRUE(shader_);
}

TEST_F(ShaderTest, FailedToLoadShaderTest)
{
    ASSERT_FALSE(shader_);
    shader_ = std::make_unique<frame::opengl::Shader>(
        frame::opengl::ShaderEnum::FRAGMENT_SHADER);
    ASSERT_TRUE(shader_);
    EXPECT_FALSE(shader_->LoadFromSource("false"));
}

TEST_F(ShaderTest, LoadFromFileVertexShaderTest)
{
    ASSERT_FALSE(shader_);
    shader_ = std::make_unique<frame::opengl::Shader>(
        frame::opengl::ShaderEnum::VERTEX_SHADER);
    ASSERT_TRUE(shader_);
    EXPECT_TRUE(shader_->LoadFromSource(GetVertexSource()));
    EXPECT_NE(0, shader_->GetId());
}

TEST_F(ShaderTest, LoadFromFileFragmentShaderTest)
{
    ASSERT_FALSE(shader_);
    shader_ = std::make_unique<frame::opengl::Shader>(
        frame::opengl::ShaderEnum::FRAGMENT_SHADER);
    ASSERT_TRUE(shader_);
    EXPECT_TRUE(shader_->LoadFromSource(GetFragmentSource()));
    EXPECT_NE(0, shader_->GetId());
}

const std::string ShaderTest::GetVertexSource() const
{
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

const std::string ShaderTest::GetFragmentSource() const
{
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

} // End namespace test.
