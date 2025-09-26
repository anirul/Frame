#include <filesystem>

#include <gtest/gtest.h>

#include "frame/file/file_system.h"
#include "frame/file/image_cache.h"

namespace test
{

TEST(ImageCacheTest, RoundTrip)
{
    frame::file::ImageCacheMetadata metadata;
    auto temp_dir =
        std::filesystem::temp_directory_path() / "frame_image_cache_roundtrip";
    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(temp_dir);
    metadata.cache_path =
        (temp_dir / "texture-RGB-BYTE-3ch.imgpb").lexically_normal();
    metadata.cache_relative = frame::file::PurifyFilePath(metadata.cache_path);
    metadata.source_relative = "asset/texture/checker.png";
    metadata.source_size = 256;
    metadata.source_mtime_ns = 1234;

    glm::ivec2 size(4, 4);
    std::vector<std::uint8_t> data(4 * 4 * 3, 42u);
    frame::file::SaveImageCache(
        metadata,
        frame::proto::PixelElementSize::BYTE,
        frame::proto::PixelStructure::RGB,
        3,
        size,
        data);

    auto loaded = frame::file::LoadImageCache(
        metadata,
        frame::proto::PixelElementSize::BYTE,
        frame::proto::PixelStructure::RGB,
        3);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->size, size);
    EXPECT_EQ(loaded->element_size, frame::proto::PixelElementSize::BYTE);
    EXPECT_EQ(loaded->structure, frame::proto::PixelStructure::RGB);
    ASSERT_EQ(loaded->data.size(), data.size());
    EXPECT_EQ(loaded->data[5], data[5]);

    std::filesystem::remove_all(temp_dir);
}

TEST(ImageCacheTest, RejectsMismatchedMetadata)
{
    frame::file::ImageCacheMetadata metadata;
    auto temp_dir =
        std::filesystem::temp_directory_path() / "frame_image_cache_stale";
    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(temp_dir);
    metadata.cache_path = (temp_dir / "texture.imgpb").lexically_normal();
    metadata.cache_relative = frame::file::PurifyFilePath(metadata.cache_path);
    metadata.source_relative = "asset/texture/checker.png";
    metadata.source_size = 256;
    metadata.source_mtime_ns = 1234;

    glm::ivec2 size(2, 2);
    std::vector<std::uint8_t> data(12u, 1u);
    frame::file::SaveImageCache(
        metadata,
        frame::proto::PixelElementSize::BYTE,
        frame::proto::PixelStructure::RGB,
        3,
        size,
        data);

    frame::file::ImageCacheMetadata stale = metadata;
    stale.source_mtime_ns += 1;
    auto should_be_empty = frame::file::LoadImageCache(
        stale,
        frame::proto::PixelElementSize::BYTE,
        frame::proto::PixelStructure::RGB,
        3);
    EXPECT_FALSE(should_be_empty.has_value());

    std::filesystem::remove_all(temp_dir);
}

} // namespace test
