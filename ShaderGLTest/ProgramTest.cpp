#include "ProgramTest.h"
#include "../ShaderGLLib/Error.h"

namespace test {

	TEST_F(ProgramTest, CreateProgramTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		sgl::Error::SetWindowPtr(nullptr);
		EXPECT_FALSE(program_);
		program_ = std::make_shared<sgl::Program>();
		EXPECT_TRUE(program_);
	}

	TEST_F(ProgramTest, CheckShaderProgramTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		sgl::Error::SetWindowPtr(nullptr);
		EXPECT_FALSE(program_);
		program_ = std::make_shared<sgl::Program>();
		EXPECT_TRUE(program_);
		sgl::Shader vertex_shader(sgl::ShaderType::VERTEX_SHADER);
		EXPECT_TRUE(vertex_shader.LoadFromFile("../Asset/Shader/Simple.vert"));
		program_->AddShader(vertex_shader);
		sgl::Shader fragment_shader(sgl::ShaderType::FRAGMENT_SHADER);
		EXPECT_TRUE(
			fragment_shader.LoadFromFile("../Asset/Shader/Simple.frag"));
		program_->AddShader(fragment_shader);
	}

	TEST_F(ProgramTest, CheckUseAndLinkProgramTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		sgl::Error::SetWindowPtr(nullptr);
		EXPECT_FALSE(program_);
		program_ = std::make_shared<sgl::Program>();
		EXPECT_TRUE(program_);
		sgl::Shader vertex_shader(sgl::ShaderType::VERTEX_SHADER);
		EXPECT_TRUE(
			vertex_shader.LoadFromFile(
				"../Asset/Shader/PhysicallyBasedRendering.vert"));
		program_->AddShader(vertex_shader);
		sgl::Shader fragment_shader(sgl::ShaderType::FRAGMENT_SHADER);
		EXPECT_TRUE(
			fragment_shader.LoadFromFile(
				"../Asset/Shader/PhysicallyBasedRendering.frag"));
		program_->AddShader(fragment_shader);
		program_->LinkShader();
		program_->Use();
	}

	// TODO(anirul): add uniform tests!

	TEST_F(ProgramTest, CreateCubeMapProgramProgramTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		sgl::Error::SetWindowPtr(nullptr);
		EXPECT_FALSE(program_);
		program_ = sgl::CreateProgram("CubeMap");
		EXPECT_TRUE(program_);
	}

	TEST_F(ProgramTest, CreateEquirectangulareCubeMapProgramTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		sgl::Error::SetWindowPtr(nullptr);
		EXPECT_FALSE(program_);
		program_ = sgl::CreateProgram("EquirectangularCubeMap");
		EXPECT_TRUE(program_);
	}

	TEST_F(ProgramTest, CreatePhysicallyBasedRenderingProgramProgramTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		sgl::Error::SetWindowPtr(nullptr);
		EXPECT_FALSE(program_);
		program_ = sgl::CreateProgram("PhysicallyBasedRendering");
		EXPECT_TRUE(program_);
	}

	TEST_F(ProgramTest, CreateSimpleProgramProgramTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		sgl::Error::SetWindowPtr(nullptr);
		EXPECT_FALSE(program_);
		program_ = sgl::CreateProgram("Simple");
		EXPECT_TRUE(program_);
	}

} // End namespace test.
