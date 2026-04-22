#include "frame/file/image_test.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#include "frame/file/file_system.h"

namespace test
{

TEST_F(ImageTest, CreateImageTest)
{
    ASSERT_FALSE(image_);
    image_ = std::make_unique<frame::file::Image>(
        frame::file::FindFile("asset/cubemap/positive_x.png"));
    EXPECT_TRUE(image_);
}

TEST_F(ImageTest, CheckSizeImageTest)
{
    ASSERT_FALSE(image_);
    image_ = std::make_unique<frame::file::Image>(
        frame::file::FindFile("asset/cubemap/positive_x.png"));
    ASSERT_TRUE(image_);
    EXPECT_EQ(1024, image_->GetSize().x);
    EXPECT_EQ(1024, image_->GetSize().y);
}

TEST_F(ImageTest, CheckTypeImageTest)
{
    ASSERT_FALSE(image_);
    image_ = std::make_unique<frame::file::Image>(
        frame::file::FindFile("asset/cubemap/positive_x.png"));
    ASSERT_TRUE(image_);
    EXPECT_TRUE(frame::json::operator==(
        frame::json::PixelElementSize_BYTE(), image_->GetPixelElementSize()));
    EXPECT_TRUE(frame::json::operator==(
        frame::json::PixelStructure_RGB(), image_->GetPixelStructure()));
}

TEST_F(ImageTest, CreateCubeMapImageTest)
{
    ASSERT_FALSE(image_);
    // This is not needed it could be strait bytes.
    image_ = std::make_unique<frame::file::Image>(
        frame::file::FindFile("asset/cubemap/hamarikyu.hdr"),
        frame::json::PixelElementSize_HALF(),
        frame::json::PixelStructure_RGB());
    EXPECT_TRUE(image_);
}

TEST_F(ImageTest, CreateCubeMapPointerImageTest)
{
    ASSERT_FALSE(image_);
    image_ = std::make_unique<frame::file::Image>(
        frame::file::FindFile("asset/cubemap/hamarikyu.hdr"),
        frame::json::PixelElementSize_FLOAT(),
        frame::json::PixelStructure_RGB_ALPHA());
    ASSERT_TRUE(image_);
    EXPECT_EQ(3200, image_->GetSize().x);
    EXPECT_EQ(1600, image_->GetSize().y);
    EXPECT_TRUE(frame::json::operator==(
        frame::json::PixelElementSize_FLOAT(), image_->GetPixelElementSize()));
    EXPECT_TRUE(frame::json::operator==(
        frame::json::PixelStructure_RGB_ALPHA(), image_->GetPixelStructure()));
    {
        const float* pointer = static_cast<float*>(image_->Data());
        const auto size = image_->GetSize();
        // Check against a random position (should be around the middle).
        const auto x = 1600;
        const auto y = 800;
        const auto position = x + y * size.x;
        // CHECKME the coordinate don't correspond I don't know why.
        // CHECKME old coordinates 581, 753 : (1.78125f, 2.4375f, 2.85938f)
        EXPECT_FLOAT_EQ(0.05078125f, pointer[position * 4]);
        EXPECT_FLOAT_EQ(0.053955078f, pointer[position * 4 + 1]);
        EXPECT_FLOAT_EQ(0.014160156f, pointer[position * 4 + 2]);
        EXPECT_FLOAT_EQ(1.0f, pointer[position * 4 + 3]);
    }
}

TEST_F(ImageTest, SaveBgrAlphaImageWritesValidPng)
{
    const auto output_file =
        std::filesystem::temp_directory_path() /
        "frame_save_bgr_alpha_image_test.png";
    std::vector<std::uint8_t> pixels = {
        0x10, 0x20, 0x30, 0x40,
        0x50, 0x60, 0x70, 0x80};

    frame::file::Image image(
        glm::uvec2(2, 1),
        frame::json::PixelElementSize_BYTE(),
        frame::json::PixelStructure_BGR_ALPHA());
    image.SetData(pixels.data());
    image.SaveImageToFile(output_file.string());

    frame::file::Image loaded_image(
        output_file,
        frame::json::PixelElementSize_BYTE(),
        frame::json::PixelStructure_RGB_ALPHA());
    const auto* loaded =
        static_cast<const std::uint8_t*>(loaded_image.Data());
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded[0], 0x30);
    EXPECT_EQ(loaded[1], 0x20);
    EXPECT_EQ(loaded[2], 0x10);
    EXPECT_EQ(loaded[3], 0x40);
    EXPECT_EQ(loaded[4], 0x70);
    EXPECT_EQ(loaded[5], 0x60);
    EXPECT_EQ(loaded[6], 0x50);
    EXPECT_EQ(loaded[7], 0x80);

    std::error_code remove_error;
    std::filesystem::remove(output_file, remove_error);
}

} // End namespace test.
