#include "Frame/Proto/Test/ParsePixelTest.h"

#include "Frame/Proto/Proto.h"
#include "Frame/Proto/ParsePixel.h"

namespace test {

	TEST_F(ParsePixelTest, CreateBYTETest)
	{
		EXPECT_EQ(1, frame::proto::PixelElementSize_BYTE().value());
	}

	TEST_F(ParsePixelTest, CreateSHORTTest)
	{
		EXPECT_EQ(2, frame::proto::PixelElementSize_SHORT().value());
	}

	TEST_F(ParsePixelTest, CreateHALFTest)
	{
		EXPECT_EQ(3, frame::proto::PixelElementSize_HALF().value());
	}

	TEST_F(ParsePixelTest, CreateFLOATTest)
	{
		EXPECT_EQ(4, frame::proto::PixelElementSize_FLOAT().value());
	}

	TEST_F(ParsePixelTest, CompareSizeTest)
	{
		EXPECT_TRUE(frame::proto::operator==(
			frame::proto::PixelElementSize_BYTE(), 
			frame::proto::PixelElementSize_BYTE()));
		EXPECT_FALSE(frame::proto::operator==(
			frame::proto::PixelElementSize_BYTE(),
			frame::proto::PixelElementSize_SHORT()));
		EXPECT_FALSE(frame::proto::operator==(
			frame::proto::PixelElementSize_BYTE(),
			frame::proto::PixelElementSize_HALF()));
		EXPECT_FALSE(frame::proto::operator==(
			frame::proto::PixelElementSize_BYTE(),
			frame::proto::PixelElementSize_FLOAT()));
		EXPECT_TRUE(frame::proto::operator==(
			frame::proto::PixelElementSize_SHORT(),
			frame::proto::PixelElementSize_SHORT()));
		EXPECT_FALSE(frame::proto::operator==(
			frame::proto::PixelElementSize_SHORT(),
			frame::proto::PixelElementSize_HALF()));
		EXPECT_FALSE(frame::proto::operator==(
			frame::proto::PixelElementSize_SHORT(),
			frame::proto::PixelElementSize_FLOAT()));
		EXPECT_TRUE(frame::proto::operator==(
			frame::proto::PixelElementSize_HALF(),
			frame::proto::PixelElementSize_HALF()));
		EXPECT_FALSE(frame::proto::operator==(
			frame::proto::PixelElementSize_HALF(),
			frame::proto::PixelElementSize_FLOAT()));
		EXPECT_TRUE(frame::proto::operator==(
			frame::proto::PixelElementSize_FLOAT(),
			frame::proto::PixelElementSize_FLOAT()));
	}

	TEST_F(ParsePixelTest, CreateGREYTest)
	{
		EXPECT_EQ(1, frame::proto::PixelStructure_GREY().value());
	}

	TEST_F(ParsePixelTest, CreateGREY_ALPHATest)
	{
		EXPECT_EQ(2, frame::proto::PixelStructure_GREY_ALPHA().value());
	}

	TEST_F(ParsePixelTest, CreateRGBTest)
	{
		EXPECT_EQ(3, frame::proto::PixelStructure_RGB().value());
	}

	TEST_F(ParsePixelTest, CreateRGB_ALPHATest)
	{
		EXPECT_EQ(4, frame::proto::PixelStructure_RGB_ALPHA().value());
	}

	TEST_F(ParsePixelTest, CompareStructureTest)
	{
		EXPECT_TRUE(frame::proto::operator==(
			frame::proto::PixelStructure_GREY(),
			frame::proto::PixelStructure_GREY()));
		EXPECT_FALSE(frame::proto::operator==(
			frame::proto::PixelStructure_GREY(),
			frame::proto::PixelStructure_GREY_ALPHA()));
		EXPECT_FALSE(frame::proto::operator==(
			frame::proto::PixelStructure_GREY(),
			frame::proto::PixelStructure_RGB()));
		EXPECT_FALSE(frame::proto::operator==(
			frame::proto::PixelStructure_GREY(),
			frame::proto::PixelStructure_RGB_ALPHA()));
		EXPECT_TRUE(frame::proto::operator==(
			frame::proto::PixelStructure_GREY_ALPHA(),
			frame::proto::PixelStructure_GREY_ALPHA()));
		EXPECT_FALSE(frame::proto::operator==(
			frame::proto::PixelStructure_GREY_ALPHA(),
			frame::proto::PixelStructure_RGB()));
		EXPECT_FALSE(frame::proto::operator==(
			frame::proto::PixelStructure_GREY_ALPHA(),
			frame::proto::PixelStructure_RGB_ALPHA()));
		EXPECT_TRUE(frame::proto::operator==(
			frame::proto::PixelStructure_RGB(),
			frame::proto::PixelStructure_RGB()));
		EXPECT_FALSE(frame::proto::operator==(
			frame::proto::PixelStructure_RGB(),
			frame::proto::PixelStructure_RGB_ALPHA()));
		EXPECT_TRUE(frame::proto::operator==(
			frame::proto::PixelStructure_RGB_ALPHA(),
			frame::proto::PixelStructure_RGB_ALPHA()));
	}

} // End namespace test.
