#include "BufferTest.h"
#include "../ShaderGLLib/Vector.h"

namespace test {

	TEST_F(BufferTest, CreationBufferTest)
	{
		EXPECT_FALSE(buffer_);
		buffer_ = std::make_shared<sgl::Buffer>();
		EXPECT_TRUE(buffer_);
	}

	TEST_F(BufferTest, CheckIDBufferTest)
	{
		ASSERT_FALSE(buffer_);
		buffer_ = std::make_shared<sgl::Buffer>();
		ASSERT_TRUE(buffer_);
		EXPECT_NE(0, buffer_->GetId());
	}

	TEST_F(BufferTest, BindBufferTest)
	{
		ASSERT_FALSE(buffer_);
		buffer_ = std::make_shared<sgl::Buffer>();
		ASSERT_TRUE(buffer_);
		buffer_->Bind();
		buffer_->UnBind();
	}

	TEST_F(BufferTest, CopyBufferTest)
	{
		ASSERT_FALSE(buffer_);
		buffer_ = std::make_shared<sgl::Buffer>();
		ASSERT_TRUE(buffer_);
		std::vector<float> test{ 4 };
		buffer_->BindCopy(test.size(), test.data());
		buffer_->UnBind();
		EXPECT_TRUE(buffer_);
	}

} // End namespace test.
