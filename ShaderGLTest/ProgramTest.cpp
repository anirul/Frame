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
		EXPECT_TRUE(vertex_shader.LoadFromFile("../Asset/Simple.Vertex.glsl"));
		program_->AddShader(vertex_shader);
		sgl::Shader fragment_shader(sgl::ShaderType::FRAGMENT_SHADER);
		EXPECT_TRUE(
			fragment_shader.LoadFromFile("../Asset/Simple.Fragment.glsl"));
		program_->AddShader(fragment_shader);
	}

	TEST_F(ProgramTest, CheckUseAndLinkProgramTest)
	{
		EXPECT_FALSE(program_);
		program_ = std::make_shared<sgl::Program>();
		EXPECT_TRUE(program_);
		sgl::Shader vertex_shader(sgl::ShaderType::VERTEX_SHADER);
		EXPECT_TRUE(vertex_shader.LoadFromFile("../Asset/PBR.Vertex.glsl"));
		program_->AddShader(vertex_shader);
		sgl::Shader fragment_shader(sgl::ShaderType::FRAGMENT_SHADER);
		EXPECT_TRUE(
			fragment_shader.LoadFromFile("../Asset/PBR.Fragment.glsl"));
		program_->AddShader(fragment_shader);
		program_->LinkShader();
		program_->Use();
	}

	// TODO(anirul): add uniform tests!

	TEST_F(ProgramTest, CreateSimpleProgramProgramTest)
	{
		EXPECT_FALSE(program_);
		program_ = sgl::CreateSimpleProgram(
			glm::mat4(1.0), 
			glm::mat4(1.0), 
			glm::mat4(1.0));
		EXPECT_TRUE(program_);
	}

	TEST_F(ProgramTest, CreatePBRProgramProgramTest)
	{
		EXPECT_FALSE(program_);
		program_ = sgl::CreatePBRProgram(
			glm::mat4(1.0),
			glm::mat4(1.0),
			glm::mat4(1.0));
		EXPECT_TRUE(program_);
	}

	TEST_F(ProgramTest, CreateCubeMapProgramProgramTest)
	{
		EXPECT_FALSE(program_);
		program_ = sgl::CreateCubeMapProgram(
			glm::mat4(1.0),
			glm::mat4(1.0),
			glm::mat4(1.0));
		EXPECT_TRUE(program_);
	}

} // End namespace test.
