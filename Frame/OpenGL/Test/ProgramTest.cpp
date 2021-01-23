#include "ProgramTest.h"
#include "Frame/Error.h"

namespace test {

	TEST_F(ProgramTest, CreateProgramTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		frame::Error::SetWindowPtr(nullptr);
		EXPECT_FALSE(program_);
		program_ = std::make_shared<frame::opengl::Program>();
		EXPECT_TRUE(program_);
	}

	TEST_F(ProgramTest, CheckShaderProgramTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		frame::Error::SetWindowPtr(nullptr);
		EXPECT_FALSE(program_);
		program_ = std::make_shared<frame::opengl::Program>();
		auto program_ptr =
			std::dynamic_pointer_cast<frame::opengl::Program>(program_);
		EXPECT_TRUE(program_);
		EXPECT_TRUE(program_ptr);
		frame::opengl::Shader vertex_shader(
			frame::opengl::ShaderEnum::VERTEX_SHADER);
		EXPECT_TRUE(
			vertex_shader.LoadFromSource(GetVertexSource()));
		program_ptr->AddShader(vertex_shader);
		frame::opengl::Shader fragment_shader(
			frame::opengl::ShaderEnum::FRAGMENT_SHADER);
		EXPECT_TRUE(
			fragment_shader.LoadFromSource(GetFragmentSource()));
		program_ptr->AddShader(fragment_shader);
	}

	TEST_F(ProgramTest, CheckUseAndLinkProgramTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		frame::Error::SetWindowPtr(nullptr);
		EXPECT_FALSE(program_);
		program_ = std::make_shared<frame::opengl::Program>();
		auto program_ptr =
			std::dynamic_pointer_cast<frame::opengl::Program>(program_);
		EXPECT_TRUE(program_);
		EXPECT_TRUE(program_ptr);
		frame::opengl::Shader vertex_shader(
			frame::opengl::ShaderEnum::VERTEX_SHADER);
		EXPECT_TRUE(
			vertex_shader.LoadFromFile(
				"../Asset/Shader/PhysicallyBasedRendering.vert"));
		program_ptr->AddShader(vertex_shader);
		frame::opengl::Shader fragment_shader(
			frame::opengl::ShaderEnum::FRAGMENT_SHADER);
		EXPECT_TRUE(
			fragment_shader.LoadFromFile(
				"../Asset/Shader/PhysicallyBasedRendering.frag"));
		program_ptr->AddShader(fragment_shader);
		program_->LinkShader();
		program_->Use();
	}

	// TODO(anirul): add uniform tests!

	TEST_F(ProgramTest, CreateCubeMapProgramProgramTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		frame::Error::SetWindowPtr(nullptr);
		EXPECT_FALSE(program_);
		program_ = frame::opengl::CreateProgram("CubeMap");
		EXPECT_TRUE(program_);
	}

	TEST_F(ProgramTest, CreateEquirectangulareCubeMapProgramTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		frame::Error::SetWindowPtr(nullptr);
		EXPECT_FALSE(program_);
		program_ = frame::opengl::CreateProgram("EquirectangularCubeMap");
		EXPECT_TRUE(program_);
	}

	TEST_F(ProgramTest, CreatePhysicallyBasedRenderingProgramProgramTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		frame::Error::SetWindowPtr(nullptr);
		EXPECT_FALSE(program_);
		program_ = frame::opengl::CreateProgram("PhysicallyBasedRendering");
		EXPECT_TRUE(program_);
	}

	TEST_F(ProgramTest, CreateSimpleProgramProgramTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		frame::Error::SetWindowPtr(nullptr);
		EXPECT_FALSE(program_);
		program_ = frame::opengl::CreateProgram("SceneSimple");
		EXPECT_TRUE(program_);
	}

} // End namespace test.
