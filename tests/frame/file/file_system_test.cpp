#include "frame/file/file_system_test.h"

#include "frame/file/file_system.h"

namespace test {

TEST_F(FileSystemTest, FindDirectorySimpleTest) {
    std::string result = frame::file::FindDirectory("asset/");
    EXPECT_FALSE(result.empty());
}

TEST_F(FileSystemTest, FindFileSimpleTest) {
    std::string result = frame::file::FindFile("asset/cubemap/positive_x.png");
    EXPECT_FALSE(result.empty());
}

TEST_F(FileSystemTest, IsDirectoryExist) {
    std::string asset = frame::file::FindDirectory("asset/");
    EXPECT_FALSE(asset.empty());
    EXPECT_TRUE(frame::file::IsDirectoryExist(asset));
}

TEST_F(FileSystemTest, IsFileExist) {
    std::string asset = frame::file::FindDirectory("asset/");
    EXPECT_FALSE(asset.empty());
    EXPECT_TRUE(frame::file::IsDirectoryExist(asset + "cubemap/"));
    EXPECT_TRUE(frame::file::IsFileExist(asset + "cubemap/positive_x.png"));
}

}  // End namespace test.
