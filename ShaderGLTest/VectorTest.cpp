#include "VectorTest.h"

namespace test {

	TEST_F(VectorTest, vector2VectorTest)
	{
		sgl::vector2 v{ 0.f, 1.f };
		EXPECT_FLOAT_EQ(0, v.x);
		EXPECT_FLOAT_EQ(1, v.y);
	}

	TEST_F(VectorTest, vector3VectorTest)
	{
		sgl::vector3 v{ 0.f, 1.f, 2.f };
		EXPECT_FLOAT_EQ(0, v.x);
		EXPECT_FLOAT_EQ(1, v.y);
		EXPECT_FLOAT_EQ(2, v.z);
	}

	TEST_F(VectorTest, vector4VectorTest)
	{
		sgl::vector4 v{ 0.f, 1.f, 2.f, 3.f };
		EXPECT_FLOAT_EQ(0, v.x);
		EXPECT_FLOAT_EQ(1, v.y);
		EXPECT_FLOAT_EQ(2, v.z);
		EXPECT_FLOAT_EQ(3, v.w);
	}

} // End namespace test.
