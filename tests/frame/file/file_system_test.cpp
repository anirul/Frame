#include "frame/file/file_system_test.h"

#include "frame/file/file_system.h"

namespace test {

TEST_F(FileSystemTest, FindDirectorySimpleTest) {
    std::filesystem::path result = frame::file::FindDirectory("asset/");
    EXPECT_FALSE(result.empty());
}

TEST_F(FileSystemTest, FindFileSimpleTest) {
    std::filesystem::path result = frame::file::FindFile("asset/cubemap/positive_x.png");
    EXPECT_FALSE(result.empty());
}

TEST_F(FileSystemTest, IsDirectoryExist) {
    std::filesystem::path asset = frame::file::FindDirectory("asset/");
    EXPECT_FALSE(asset.empty());
    EXPECT_TRUE(std::filesystem::is_directory(asset));
}

TEST_F(FileSystemTest, IsFileExist) {
    std::filesystem::path asset = frame::file::FindDirectory("asset/");
    EXPECT_FALSE(asset.empty());
    EXPECT_TRUE(std::filesystem::is_directory(asset / "cubemap/"));
    EXPECT_TRUE(std::filesystem::is_regular_file(asset / "cubemap/positive_x.png"));
}

}  // End namespace test.
