#include "frame/bvh.h"

#include <gtest/gtest.h>

namespace test
{

TEST(BvhTest, BuildBalancedTree)
{
    // Four triangles forming two squares
    std::vector<float> points = {
        0.f, 0.f, 0.f,
        1.f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        1.f, 1.f, 0.f,
        2.f, 0.f, 0.f,
        2.f, 1.f, 0.f,
        3.f, 0.f, 0.f,
        3.f, 1.f, 0.f,
    };
    std::vector<std::uint32_t> indices = {
        0, 1, 2,
        1, 3, 2,
        4, 5, 6,
        5, 7, 6,
    };
    auto nodes = frame::BuildBVH(points, indices);
    EXPECT_EQ(nodes.size(), 7);
    int leaf = 0;
    for (const auto& n : nodes)
    {
        if (n.triangle_count == 1)
        {
            ++leaf;
            EXPECT_EQ(n.left, -1);
            EXPECT_EQ(n.right, -1);
        }
    }
    EXPECT_EQ(leaf, 4);
    EXPECT_NEAR(nodes[0].min.x, 0.f, 1e-5);
    EXPECT_NEAR(nodes[0].max.x, 3.f, 1e-5);
}

} // namespace test

