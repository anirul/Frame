#include "BufferTest.h"
#include "../ShaderGLLib/Vector.h"

namespace test {

	TEST_F(BufferTest, CreationBufferTest)
	{
		EXPECT_FALSE(buffer_);
		buffer_ = std::make_shared<sgl::Buffer>();
		EXPECT_TRUE(buffer_);
	}

	TEST_F(BufferTest, BufferDataTest)
	{
		EXPECT_FALSE(buffer_);
		buffer_ = std::make_shared<sgl::Buffer>();
		EXPECT_TRUE(buffer_);
		std::vector<float> test{ 4 };
		buffer_->BindCopy(test.size(), test.data());
		EXPECT_TRUE(buffer_);
	}

	TEST_F(BufferTest, UniformBufferTest)
	{
		EXPECT_FALSE(buffer_);
		buffer_ = std::make_shared<sgl::Buffer>(
			sgl::BufferType::UNIFORM_BUFFER,
			sgl::BufferUsage::DYNAMIC_DRAW);
		EXPECT_TRUE(buffer_);
		buffer_->BindCopy(sizeof(float) * 4);
		EXPECT_TRUE(buffer_);
		{
			auto* temp =
				static_cast<sgl::vector4*>(
					buffer_->BindMap(sizeof(float) * 4));
			ASSERT_TRUE(temp);
			temp->x = 1;
			temp->y = 3;
			temp->z = 3;
			temp->w = 7;
			buffer_->UnBindUnMap();
		}
		EXPECT_TRUE(buffer_);
		{
			auto* temp =
				static_cast<sgl::vector4*>(
					buffer_->BindMap(sizeof(float) * 4));
			ASSERT_TRUE(temp);
			EXPECT_FLOAT_EQ(1, temp->x);
			EXPECT_FLOAT_EQ(3, temp->y);
			EXPECT_FLOAT_EQ(3, temp->z);
			EXPECT_FLOAT_EQ(7, temp->w);
			buffer_->UnBindUnMap();
		}
		EXPECT_TRUE(buffer_);
	}

} // End namespace test.
