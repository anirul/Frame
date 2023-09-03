#include "frame/file/image_test.h"

#include <memory>

#include "frame/file/file_system.h"

namespace test {

    TEST_F(ImageTest, CreateImageTest) {
        ASSERT_FALSE(image_);
        image_ = std::make_unique<frame::file::Image>(
            frame::file::FindFile("asset/cubemap/positive_x.png"));
        EXPECT_TRUE(image_);
    }

    TEST_F(ImageTest, CheckSizeImageTest) {
        ASSERT_FALSE(image_);
        image_ = std::make_unique<frame::file::Image>(
            frame::file::FindFile("asset/cubemap/positive_x.png"));
        ASSERT_TRUE(image_);
        EXPECT_EQ(1024, image_->GetSize().x);
        EXPECT_EQ(1024, image_->GetSize().y);
    }

    TEST_F(ImageTest, CheckTypeImageTest) {
        ASSERT_FALSE(image_);
        image_ = std::make_unique<frame::file::Image>(
            frame::file::FindFile("asset/cubemap/positive_x.png"));
        ASSERT_TRUE(image_);
        EXPECT_EQ(frame::proto::PixelElementSize_BYTE(),
            image_->GetPixelElementSize());
        EXPECT_EQ(
            frame::proto::PixelStructure_RGB(),
            image_->GetPixelStructure());
    }

    TEST_F(ImageTest, CreateCubeMapImageTest) {
        ASSERT_FALSE(image_);
        // This is not needed it could be strait bytes.
        image_ = std::make_unique<frame::file::Image>(
            frame::file::FindFile("asset/cubemap/hamarikyu.hdr"),
            frame::proto::PixelElementSize_HALF(),
            frame::proto::PixelStructure_RGB());
        EXPECT_TRUE(image_);
    }

    TEST_F(ImageTest, CreateCubeMapPointerImageTest) {
        ASSERT_FALSE(image_);
        image_ = std::make_unique<frame::file::Image>(
            frame::file::FindFile("asset/cubemap/hamarikyu.hdr"),
            frame::proto::PixelElementSize_FLOAT(),
            frame::proto::PixelStructure_RGB_ALPHA());
        ASSERT_TRUE(image_);
        EXPECT_EQ(3200, image_->GetSize().x);
        EXPECT_EQ(1600, image_->GetSize().y);
        EXPECT_EQ(frame::proto::PixelElementSize_FLOAT(),
            image_->GetPixelElementSize());
        EXPECT_EQ(frame::proto::PixelStructure_RGB_ALPHA(),
            image_->GetPixelStructure());
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

}  // End namespace test.
