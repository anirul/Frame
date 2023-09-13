#include "frame/file/ply_test.h"

#include "frame/file/file_system.h"

namespace test
{

TEST_F(PlyTest, CreatePlyTest)
{
    ASSERT_FALSE(ply_);
    ply_ = std::make_unique<frame::file::Ply>(
        frame::file::FindFile("asset/model/cube.ply"));
    EXPECT_TRUE(ply_);
}

TEST_F(PlyTest, GetElementPlyTest)
{
    ASSERT_FALSE(ply_);
    ply_ = std::make_unique<frame::file::Ply>(
        frame::file::FindFile("asset/model/apple.ply"));
    EXPECT_TRUE(ply_);
    auto vertices_vec = ply_->GetVertices();
    EXPECT_NE(0, vertices_vec.size());
    auto normals_vec = ply_->GetNormals();
    EXPECT_NE(0, normals_vec.size());
}

} // End namespace test.
