#include "ProgramTest.h"

namespace test {

	TEST_F(ProgramTest, CreateProgramTest)
	{
		EXPECT_FALSE(program_);
		program_ = std::make_shared<sgl::Program>();
		EXPECT_TRUE(program_);
	}

	TEST_F(ProgramTest, CheckShaderProgramTest)
	{
		EXPECT_FALSE(program_);
		program_ = std::make_shared<sgl::Program>();
		EXPECT_TRUE(program_);
		sgl::Shader vertex_shader(sgl::ShaderType::VERTEX_SHADER);
		EXPECT_TRUE(vertex_shader.LoadFromFile("../Asset/SimpleVertex.glsl"));
		program_->AddShader(vertex_shader);
		sgl::Shader fragment_shader(sgl::ShaderType::FRAGMENT_SHADER);
		EXPECT_TRUE(
			fragment_shader.LoadFromFile("../Asset/SimpleFragment.glsl"));
		program_->AddShader(fragment_shader);
	}

	TEST_F(ProgramTest, CheckUseAndLinkProgramTest)
	{
		EXPECT_FALSE(program_);
		program_ = std::make_shared<sgl::Program>();
		EXPECT_TRUE(program_);
		sgl::Shader vertex_shader(sgl::ShaderType::VERTEX_SHADER);
		EXPECT_TRUE(vertex_shader.LoadFromFile("../Asset/SimpleVertex.glsl"));
		program_->AddShader(vertex_shader);
		sgl::Shader fragment_shader(sgl::ShaderType::FRAGMENT_SHADER);
		EXPECT_TRUE(
			fragment_shader.LoadFromFile("../Asset/SimpleFragment.glsl"));
		program_->AddShader(fragment_shader);
		program_->LinkShader();
		program_->Use();
	}

} // End namespace test.
