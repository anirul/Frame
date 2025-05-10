#include "frame/json/parse_pixel_test.h"

#include "frame/json/parse_pixel.h"
#include "frame/json/proto.h"

namespace test
{

TEST_F(ParsePixelTest, CreateBYTETest)
{
    EXPECT_EQ(1, frame::json::PixelElementSize_BYTE().value());
}

TEST_F(ParsePixelTest, CreateSHORTTest)
{
    EXPECT_EQ(2, frame::json::PixelElementSize_SHORT().value());
}

TEST_F(ParsePixelTest, CreateHALFTest)
{
    EXPECT_EQ(3, frame::json::PixelElementSize_HALF().value());
}

TEST_F(ParsePixelTest, CreateFLOATTest)
{
    EXPECT_EQ(4, frame::json::PixelElementSize_FLOAT().value());
}

TEST_F(ParsePixelTest, CompareSizeTest)
{
    EXPECT_TRUE(frame::json::operator==(
        frame::json::PixelElementSize_BYTE(),
        frame::json::PixelElementSize_BYTE()));
    EXPECT_FALSE(frame::json::operator==(
        frame::json::PixelElementSize_BYTE(),
        frame::json::PixelElementSize_SHORT()));
    EXPECT_FALSE(frame::json::operator==(
        frame::json::PixelElementSize_BYTE(),
        frame::json::PixelElementSize_HALF()));
    EXPECT_FALSE(frame::json::operator==(
        frame::json::PixelElementSize_BYTE(),
        frame::json::PixelElementSize_FLOAT()));
    EXPECT_TRUE(frame::json::operator==(
        frame::json::PixelElementSize_SHORT(),
        frame::json::PixelElementSize_SHORT()));
    EXPECT_FALSE(frame::json::operator==(
        frame::json::PixelElementSize_SHORT(),
        frame::json::PixelElementSize_HALF()));
    EXPECT_FALSE(frame::json::operator==(
        frame::json::PixelElementSize_SHORT(),
        frame::json::PixelElementSize_FLOAT()));
    EXPECT_TRUE(frame::json::operator==(
        frame::json::PixelElementSize_HALF(),
        frame::json::PixelElementSize_HALF()));
    EXPECT_FALSE(frame::json::operator==(
        frame::json::PixelElementSize_HALF(),
        frame::json::PixelElementSize_FLOAT()));
    EXPECT_TRUE(frame::json::operator==(
        frame::json::PixelElementSize_FLOAT(),
        frame::json::PixelElementSize_FLOAT()));
}

TEST_F(ParsePixelTest, CreateGREYTest)
{
    EXPECT_EQ(1, frame::json::PixelStructure_GREY().value());
}

TEST_F(ParsePixelTest, CreateGREY_ALPHATest)
{
    EXPECT_EQ(2, frame::json::PixelStructure_GREY_ALPHA().value());
}

TEST_F(ParsePixelTest, CreateRGBTest)
{
    EXPECT_EQ(3, frame::json::PixelStructure_RGB().value());
}

TEST_F(ParsePixelTest, CreateRGB_ALPHATest)
{
    EXPECT_EQ(4, frame::json::PixelStructure_RGB_ALPHA().value());
}

TEST_F(ParsePixelTest, CompareStructureTest)
{
    EXPECT_TRUE(frame::json::operator==(
        frame::json::PixelStructure_GREY(),
        frame::json::PixelStructure_GREY()));
    EXPECT_FALSE(frame::json::operator==(
        frame::json::PixelStructure_GREY(),
        frame::json::PixelStructure_GREY_ALPHA()));
    EXPECT_FALSE(frame::json::operator==(
        frame::json::PixelStructure_GREY(),
        frame::json::PixelStructure_RGB()));
    EXPECT_FALSE(frame::json::operator==(
        frame::json::PixelStructure_GREY(),
        frame::json::PixelStructure_RGB_ALPHA()));
    EXPECT_TRUE(frame::json::operator==(
        frame::json::PixelStructure_GREY_ALPHA(),
        frame::json::PixelStructure_GREY_ALPHA()));
    EXPECT_FALSE(frame::json::operator==(
        frame::json::PixelStructure_GREY_ALPHA(),
        frame::json::PixelStructure_RGB()));
    EXPECT_FALSE(frame::json::operator==(
        frame::json::PixelStructure_GREY_ALPHA(),
        frame::json::PixelStructure_RGB_ALPHA()));
    EXPECT_TRUE(frame::json::operator==(
        frame::json::PixelStructure_RGB(),
        frame::json::PixelStructure_RGB()));
    EXPECT_FALSE(frame::json::operator==(
        frame::json::PixelStructure_RGB(),
        frame::json::PixelStructure_RGB_ALPHA()));
    EXPECT_TRUE(frame::json::operator==(
        frame::json::PixelStructure_RGB_ALPHA(),
        frame::json::PixelStructure_RGB_ALPHA()));
}

} // End namespace test.
