#include "frame/bvh.h"

#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <numeric>

namespace frame
{

namespace
{

constexpr int kBinCount = 12;
constexpr float kMinExtent = 1e-5f;
constexpr float kTraversalCost = 1.0f;
constexpr float kTriangleCost = 1.0f;

struct BuildTriangle
{
    AABB bounds;
    glm::vec3 centroid;
};

struct Bin
{
    AABB bounds;
    int count{0};
};

float SurfaceArea(const AABB& bounds)
{
    if (bounds.max.x < bounds.min.x || bounds.max.y < bounds.min.y ||
        bounds.max.z < bounds.min.z)
    {
        return 0.0f;
    }
    glm::vec3 extent = bounds.max - bounds.min;
    return 2.0f *
           (extent.x * extent.y + extent.x * extent.z + extent.y * extent.z);
}

} // namespace

std::vector<BVHNode> BuildBVH(
    const std::vector<float>& points, const std::vector<std::uint32_t>& indices)
{
    const int tri_count = static_cast<int>(indices.size() / 3);
    std::vector<BuildTriangle> tris(static_cast<std::size_t>(tri_count));
    for (int i = 0; i < tri_count; ++i)
    {
        glm::vec3 v0{
            points[indices[i * 3] * 3 + 0],
            points[indices[i * 3] * 3 + 1],
            points[indices[i * 3] * 3 + 2]};
        glm::vec3 v1{
            points[indices[i * 3 + 1] * 3 + 0],
            points[indices[i * 3 + 1] * 3 + 1],
            points[indices[i * 3 + 1] * 3 + 2]};
        glm::vec3 v2{
            points[indices[i * 3 + 2] * 3 + 0],
            points[indices[i * 3 + 2] * 3 + 1],
            points[indices[i * 3 + 2] * 3 + 2]};
        tris[i].bounds.expand(v0);
        tris[i].bounds.expand(v1);
        tris[i].bounds.expand(v2);
        tris[i].centroid = (v0 + v1 + v2) / 3.0f;
    }

    std::vector<int> tri_indices(static_cast<std::size_t>(tri_count));
    std::iota(tri_indices.begin(), tri_indices.end(), 0);

    std::vector<BVHNode> nodes;
    nodes.reserve(static_cast<std::size_t>(tri_count) * 2);

    std::function<int(int, int)> build = [&](int start, int end) -> int {
        AABB bounds;
        for (int i = start; i < end; ++i)
        {
            bounds.expand(tris[tri_indices[i]].bounds);
        }

        BVHNode node;
        node.min = bounds.min;
        node.max = bounds.max;
        const int node_index = static_cast<int>(nodes.size());
        nodes.push_back(node);

        const int count = end - start;
        if (count == 1)
        {
            node.first_triangle = tri_indices[start];
            node.triangle_count = 1;
            nodes[node_index] = node;
            return node_index;
        }

        AABB centroid_bounds;
        for (int i = start; i < end; ++i)
        {
            centroid_bounds.expand(tris[tri_indices[i]].centroid);
        }

        int axis = 0;
        glm::vec3 centroid_extent = centroid_bounds.max - centroid_bounds.min;
        if (centroid_extent.y > centroid_extent.x)
            axis = 1;
        if (centroid_extent.z > centroid_extent[axis])
            axis = 2;

        float best_cost = std::numeric_limits<float>::max();
        int best_axis = -1;
        int best_bin = -1;
        const float parent_sa = SurfaceArea(bounds);
        const float inv_parent_sa = parent_sa > 0.0f ? 1.0f / parent_sa : 0.0f;

        for (int test_axis = 0; test_axis < 3; ++test_axis)
        {
            const float extent =
                centroid_bounds.max[test_axis] - centroid_bounds.min[test_axis];
            if (extent <= kMinExtent)
                continue;

            std::array<Bin, kBinCount> bins{};
            const float scale = static_cast<float>(kBinCount) / extent;
            for (int i = start; i < end; ++i)
            {
                const int tri_idx = tri_indices[i];
                float offset = (tris[tri_idx].centroid[test_axis] -
                                centroid_bounds.min[test_axis]) *
                               scale;
                int bin =
                    std::clamp(static_cast<int>(offset), 0, kBinCount - 1);
                bins[bin].bounds.expand(tris[tri_idx].bounds);
                bins[bin].count++;
            }

            std::array<AABB, kBinCount> left_bounds{};
            std::array<int, kBinCount> left_counts{};
            std::array<AABB, kBinCount> right_bounds{};
            std::array<int, kBinCount> right_counts{};

            AABB accum_bounds;
            int accum_count = 0;
            for (int i = 0; i < kBinCount; ++i)
            {
                if (bins[i].count > 0)
                {
                    accum_bounds.expand(bins[i].bounds);
                    accum_count += bins[i].count;
                }
                left_bounds[i] = accum_bounds;
                left_counts[i] = accum_count;
            }

            accum_bounds = AABB{};
            accum_count = 0;
            for (int i = kBinCount - 1; i >= 0; --i)
            {
                if (bins[i].count > 0)
                {
                    accum_bounds.expand(bins[i].bounds);
                    accum_count += bins[i].count;
                }
                right_bounds[i] = accum_bounds;
                right_counts[i] = accum_count;
            }

            for (int split = 0; split < kBinCount - 1; ++split)
            {
                const int left_count = left_counts[split];
                const int right_count = right_counts[split + 1];
                if (left_count == 0 || right_count == 0)
                    continue;

                const float left_sa = SurfaceArea(left_bounds[split]);
                const float right_sa = SurfaceArea(right_bounds[split + 1]);
                float sah_cost =
                    (left_sa * left_count + right_sa * right_count) *
                        inv_parent_sa * kTriangleCost +
                    kTraversalCost;
                if (inv_parent_sa == 0.0f)
                {
                    sah_cost = static_cast<float>(left_count + right_count);
                }

                if (sah_cost < best_cost)
                {
                    best_cost = sah_cost;
                    best_axis = test_axis;
                    best_bin = split;
                }
            }
        }

        bool used_sah_split = false;
        int mid = start + count / 2;

        if (best_axis >= 0)
        {
            const float extent =
                centroid_bounds.max[best_axis] - centroid_bounds.min[best_axis];
            if (extent > kMinExtent)
            {
                const float split_pos =
                    centroid_bounds.min[best_axis] +
                    extent * (static_cast<float>(best_bin + 1) /
                              static_cast<float>(kBinCount));
                auto begin = tri_indices.begin() + start;
                auto end_it = tri_indices.begin() + end;
                auto pivot = std::partition(begin, end_it, [&](int idx) {
                    return tris[idx].centroid[best_axis] < split_pos;
                });
                mid = start + static_cast<int>(pivot - begin);
                if (mid > start && mid < end)
                {
                    axis = best_axis;
                    used_sah_split = true;
                }
            }
        }

        if (!used_sah_split)
        {
            mid = start + count / 2;
            std::nth_element(
                tri_indices.begin() + start,
                tri_indices.begin() + mid,
                tri_indices.begin() + end,
                [&](int a, int b) {
                    return tris[a].centroid[axis] < tris[b].centroid[axis];
                });
        }

        node.left = build(start, mid);
        node.right = build(mid, end);
        nodes[node_index] = node;
        return node_index;
    };

    if (tri_count > 0)
        build(0, tri_count);
    return nodes;
}

} // namespace frame
