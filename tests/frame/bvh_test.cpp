#include "frame/bvh.h"
#include "frame/bvh_cache.h"
#include "frame/file/file_system.h"

#include <filesystem>

#include <gtest/gtest.h>

namespace test
{

TEST(BvhTest, BuildBalancedTree)
{
    // Four triangles forming two squares
    std::vector<float> points = {
        0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f,
        2.f, 0.f, 0.f, 2.f, 1.f, 0.f, 3.f, 0.f, 0.f, 3.f, 1.f, 0.f,
    };
    std::vector<std::uint32_t> indices = {
        0,
        1,
        2,
        1,
        3,
        2,
        4,
        5,
        6,
        5,
        7,
        6,
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

TEST(BvhCacheTest, RoundTrip)
{
    std::vector<frame::BVHNode> nodes;
    frame::BVHNode node_a;
    node_a.min = {0.f, 1.f, 2.f};
    node_a.max = {3.f, 4.f, 5.f};
    node_a.left = -1;
    node_a.right = -1;
    node_a.first_triangle = 10;
    node_a.triangle_count = 1;
    nodes.push_back(node_a);
    frame::BVHNode node_b;
    node_b.min = {-1.f, -1.f, -1.f};
    node_b.max = {1.f, 1.f, 1.f};
    node_b.left = 0;
    node_b.right = 0;
    node_b.first_triangle = 0;
    node_b.triangle_count = 2;
    nodes.push_back(node_b);

    auto temp_dir = std::filesystem::temp_directory_path() /
                    "frame_bvh_cache_test_roundtrip";
    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(temp_dir);
    auto cache_path = temp_dir / "dragon-0.bvhpb";
    frame::BvhCacheMetadata metadata;
    metadata.cache_path = cache_path;
    metadata.cache_relative = frame::file::PurifyFilePath(cache_path);
    metadata.source_relative = "asset/model/dragon.obj";
    metadata.source_size = 42;
    metadata.source_mtime_ns = 123456789ull;

    frame::SaveBvhCache(metadata, nodes);
    auto loaded = frame::LoadBvhCache(metadata);
    ASSERT_TRUE(loaded.has_value());
    ASSERT_EQ(loaded->size(), nodes.size());
    EXPECT_EQ((*loaded)[0].first_triangle, nodes[0].first_triangle);
    EXPECT_EQ((*loaded)[1].triangle_count, nodes[1].triangle_count);

    std::filesystem::remove_all(temp_dir);
}

TEST(BvhCacheTest, RejectsOutdatedMetadata)
{
    std::vector<frame::BVHNode> nodes(1);
    nodes[0].triangle_count = 1;
    auto temp_dir =
        std::filesystem::temp_directory_path() / "frame_bvh_cache_test_stale";
    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(temp_dir);
    auto cache_path = temp_dir / "dragon-0.bvhpb";
    frame::BvhCacheMetadata metadata;
    metadata.cache_path = cache_path;
    metadata.cache_relative = frame::file::PurifyFilePath(cache_path);
    metadata.source_relative = "asset/model/dragon.obj";
    metadata.source_size = 5;
    metadata.source_mtime_ns = 1000;
    frame::SaveBvhCache(metadata, nodes);

    frame::BvhCacheMetadata stale = metadata;
    stale.source_size = metadata.source_size + 1;
    auto result = frame::LoadBvhCache(stale);
    EXPECT_FALSE(result.has_value());

    std::filesystem::remove_all(temp_dir);
}

} // namespace test
