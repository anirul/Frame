#include "BufferTest.h"
#include "Frame/OpenGL/Buffer.h"

namespace test {

	TEST_F(BufferTest, CreationBufferTest)
	{
		EXPECT_FALSE(buffer_);
		EXPECT_EQ(GLEW_OK, glewInit());
		buffer_ = std::make_unique<frame::opengl::Buffer>();
		EXPECT_TRUE(buffer_);
	}

	TEST_F(BufferTest, CheckIDBufferTest)
	{
		ASSERT_FALSE(buffer_);
		EXPECT_EQ(GLEW_OK, glewInit());
		buffer_ = std::make_unique<frame::opengl::Buffer>();
		ASSERT_TRUE(buffer_);
		EXPECT_NE(0, buffer_->GetId());
	}

	TEST_F(BufferTest, BindBufferTest)
	{
		ASSERT_FALSE(buffer_);
		EXPECT_EQ(GLEW_OK, glewInit());
		buffer_ = std::make_unique<frame::opengl::Buffer>();
		ASSERT_TRUE(buffer_);
		buffer_->Bind();
		buffer_->UnBind();
	}

	TEST_F(BufferTest, CopyBufferTest)
	{
		ASSERT_FALSE(buffer_);
		EXPECT_EQ(GLEW_OK, glewInit());
		buffer_ = std::make_unique<frame::opengl::Buffer>();
		ASSERT_TRUE(buffer_);
		std::vector<float> test(4, 1.0f);
		buffer_->Bind();
		buffer_->Copy(test.size() * sizeof(float), test.data());
		EXPECT_EQ(4 * sizeof(float), buffer_->GetSize());
		buffer_->UnBind();
		EXPECT_TRUE(buffer_);
	}

} // End namespace test.
