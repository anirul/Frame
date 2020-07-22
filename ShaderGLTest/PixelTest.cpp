#include "PixelTest.h"
#include <GL/glew.h>

namespace test {

	TEST_F(PixelTest, ConvertToGLTypePixelElementPixelTest)
	{
		EXPECT_EQ(
			GL_UNSIGNED_BYTE, 
			sgl::ConvertToGLType(sgl::PixelElementSize_BYTE()));
		EXPECT_EQ(
			GL_UNSIGNED_SHORT,
			sgl::ConvertToGLType(sgl::PixelElementSize_SHORT()));
		EXPECT_EQ(
			GL_FLOAT,
			sgl::ConvertToGLType(sgl::PixelElementSize_HALF()));
		EXPECT_EQ(
			GL_FLOAT,
			sgl::ConvertToGLType(sgl::PixelElementSize_FLOAT()));
	}

	TEST_F(PixelTest, ConvertToGLTypePixelStructurePixelTest)
	{
		EXPECT_EQ(
			GL_RED, 
			sgl::ConvertToGLType(sgl::PixelStructure_GREY()));
		EXPECT_EQ(
			GL_RG, 
			sgl::ConvertToGLType(sgl::PixelStructure_GREY_ALPHA()));
		EXPECT_EQ(
			GL_RGB, 
			sgl::ConvertToGLType(sgl::PixelStructure_RGB()));
		EXPECT_EQ(
			GL_RGBA, 
			sgl::ConvertToGLType(sgl::PixelStructure_RGB_ALPHA()));
	}

	TEST_F(PixelTest, ConvertToGLTypePixelElementAndStructurePixelTest)
	{
		EXPECT_EQ(
			GL_R8, 
			sgl::ConvertToGLType(
				sgl::PixelElementSize_BYTE(), 
				sgl::PixelStructure_GREY()));
		EXPECT_EQ(
			GL_RG8,
			sgl::ConvertToGLType(
				sgl::PixelElementSize_BYTE(),
				sgl::PixelStructure_GREY_ALPHA()));
		EXPECT_EQ(
			GL_RGB8,
			sgl::ConvertToGLType(
				sgl::PixelElementSize_BYTE(),
				sgl::PixelStructure_RGB()));
		EXPECT_EQ(
			GL_RGBA8,
			sgl::ConvertToGLType(
				sgl::PixelElementSize_BYTE(),
				sgl::PixelStructure_RGB_ALPHA()));

		EXPECT_EQ(
			GL_R16,
			sgl::ConvertToGLType(
				sgl::PixelElementSize_SHORT(),
				sgl::PixelStructure_GREY()));
		EXPECT_EQ(
			GL_RG16,
			sgl::ConvertToGLType(
				sgl::PixelElementSize_SHORT(),
				sgl::PixelStructure_GREY_ALPHA()));
		EXPECT_EQ(
			GL_RGB16,
			sgl::ConvertToGLType(
				sgl::PixelElementSize_SHORT(),
				sgl::PixelStructure_RGB()));
		EXPECT_EQ(
			GL_RGBA16,
			sgl::ConvertToGLType(
				sgl::PixelElementSize_SHORT(),
				sgl::PixelStructure_RGB_ALPHA()));

		EXPECT_EQ(
			GL_R32F,
			sgl::ConvertToGLType(
				sgl::PixelElementSize_FLOAT(),
				sgl::PixelStructure_GREY()));
		EXPECT_EQ(
			GL_RG32F,
			sgl::ConvertToGLType(
				sgl::PixelElementSize_FLOAT(),
				sgl::PixelStructure_GREY_ALPHA()));
		EXPECT_EQ(
			GL_RGB32F,
			sgl::ConvertToGLType(
				sgl::PixelElementSize_FLOAT(),
				sgl::PixelStructure_RGB()));
		EXPECT_EQ(
			GL_RGBA32F,
			sgl::ConvertToGLType(
				sgl::PixelElementSize_FLOAT(),
				sgl::PixelStructure_RGB_ALPHA()));
	}

} // End namespace test.
