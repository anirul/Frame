#include "frame/file/file_system_test.h"

#include "frame/file/file_system.h"

namespace test
{

TEST_F(FileSystemTest, FindDirectorySimpleTest)
{
    std::filesystem::path result = frame::file::FindDirectory("asset/");
    EXPECT_FALSE(result.empty());
}

TEST_F(FileSystemTest, FindFileSimpleTest)
{
    std::filesystem::path result =
        frame::file::FindFile("asset/cubemap/positive_x.png");
    EXPECT_FALSE(result.empty());
}

TEST_F(FileSystemTest, IsDirectoryExist)
{
    std::filesystem::path asset = frame::file::FindDirectory("asset/");
    EXPECT_FALSE(asset.empty());
    EXPECT_TRUE(std::filesystem::is_directory(asset));
}

TEST_F(FileSystemTest, IsFileExist)
{
    std::filesystem::path asset = frame::file::FindDirectory("asset/");
    EXPECT_FALSE(asset.empty());
    std::filesystem::path cubemap =
        frame::file::FindDirectory("asset/cubemap/");
    EXPECT_FALSE(cubemap.empty());
    EXPECT_TRUE(std::filesystem::is_directory(cubemap));
    std::filesystem::path file =
        frame::file::FindFile("asset/cubemap/positive_x.png");
    EXPECT_FALSE(file.empty());
    EXPECT_TRUE(std::filesystem::is_regular_file(file));
}

} // End namespace test.
