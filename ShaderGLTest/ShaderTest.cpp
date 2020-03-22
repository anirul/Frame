#include "ShaderTest.h"

namespace test {

	TEST_F(ShaderTest, CreateVertexShaderTest)
	{
		EXPECT_FALSE(shader_);
		shader_ = std::make_shared<sgl::Shader>(sgl::ShaderType::VERTEX_SHADER);
		EXPECT_TRUE(shader_);
	}

	TEST_F(ShaderTest, CreateFragmentShaderTest)
	{
		EXPECT_FALSE(shader_);
		shader_ = 
			std::make_shared<sgl::Shader>(sgl::ShaderType::FRAGMENT_SHADER);
		EXPECT_TRUE(shader_);
	}

	TEST_F(ShaderTest, LoadFromFileVertexShaderTest)
	{
		EXPECT_FALSE(shader_);
		shader_ = std::make_shared<sgl::Shader>(sgl::ShaderType::VERTEX_SHADER);
		EXPECT_TRUE(shader_);
		shader_->LoadFromFile("../Asset/SimpleVertex.glsl");
		EXPECT_NE(0, shader_->GetId());
	}

	TEST_F(ShaderTest, LoadFromFileFragmentShaderTest)
	{
		EXPECT_FALSE(shader_);
		shader_ = 
			std::make_shared<sgl::Shader>(sgl::ShaderType::FRAGMENT_SHADER);
		EXPECT_TRUE(shader_);
		shader_->LoadFromFile("../Asset/SimpleFragment.glsl");
		EXPECT_NE(0, shader_->GetId());
	}

} // End namespace test.
