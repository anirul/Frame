#include "FileSystemTest.h"

#include "Frame/File/FileSystem.h"

namespace test {

TEST_F(FileSystemTest, FindDirectorySimpleTest) {
    std::string result = frame::file::FindDirectory("Asset/");
    EXPECT_FALSE(result.empty());
}

TEST_F(FileSystemTest, FindFileSimpleTest) {
    std::string result = frame::file::FindFile("Asset/CubeMap/PositiveX.png");
    EXPECT_FALSE(result.empty());
}

TEST_F(FileSystemTest, IsDirectoryExist) {
    std::string asset = frame::file::FindDirectory("Asset/");
    EXPECT_FALSE(asset.empty());
    EXPECT_TRUE(frame::file::IsDirectoryExist(asset));
}

TEST_F(FileSystemTest, IsFileExist) {
    std::string asset = frame::file::FindDirectory("Asset/");
    EXPECT_FALSE(asset.empty());
    EXPECT_TRUE(frame::file::IsDirectoryExist(asset + "CubeMap/"));
    EXPECT_TRUE(frame::file::IsFileExist(asset + "CubeMap/PositiveX.png"));
}

}  // End namespace test.
