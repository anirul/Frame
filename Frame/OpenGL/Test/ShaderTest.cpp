#include "ShaderTest.h"

namespace test {

	TEST_F(ShaderTest, CreateVertexShaderTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(shader_);
		shader_ = std::make_shared<frame::opengl::Shader>(
			frame::opengl::ShaderEnum::VERTEX_SHADER);
		EXPECT_TRUE(shader_);
	}

	TEST_F(ShaderTest, CreateFragmentShaderTest)
	{
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_FALSE(shader_);
		shader_ = std::make_shared<frame::opengl::Shader>(
			frame::opengl::ShaderEnum::FRAGMENT_SHADER);
		EXPECT_TRUE(shader_);
	}

	TEST_F(ShaderTest, FailedToLoadShaderTest)
	{
		ASSERT_EQ(GLEW_OK, glewInit());
		ASSERT_FALSE(shader_);
		shader_ = std::make_shared<frame::opengl::Shader>(
			frame::opengl::ShaderEnum::FRAGMENT_SHADER);
		ASSERT_TRUE(shader_);
		EXPECT_FALSE(shader_->LoadFromSource("false"));
	}

	TEST_F(ShaderTest, LoadFromFileVertexShaderTest)
	{
		ASSERT_EQ(GLEW_OK, glewInit());
		ASSERT_FALSE(shader_);
		shader_ = std::make_shared<frame::opengl::Shader>(
			frame::opengl::ShaderEnum::VERTEX_SHADER);
		ASSERT_TRUE(shader_);
		EXPECT_TRUE(shader_->LoadFromFile("../Asset/Shader/SceneSimple.vert"));
		EXPECT_NE(0, shader_->GetId());
	}

	TEST_F(ShaderTest, LoadFromFileFragmentShaderTest)
	{
		ASSERT_EQ(GLEW_OK, glewInit());
		ASSERT_FALSE(shader_);
		shader_ = std::make_shared<frame::opengl::Shader>(
			frame::opengl::ShaderEnum::FRAGMENT_SHADER);
		ASSERT_TRUE(shader_);
		EXPECT_TRUE(shader_->LoadFromFile("../Asset/Shader/SceneSimple.frag"));
		EXPECT_NE(0, shader_->GetId());
	}

} // End namespace test.
