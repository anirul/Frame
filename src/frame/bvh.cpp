#include "frame/bvh.h"

#include <algorithm>
#include <numeric>
#include <functional>

namespace frame
{

namespace
{
struct BuildTriangle
{
    AABB bounds;
    glm::vec3 centroid;
};
} // namespace

std::vector<BVHNode> BuildBVH(
    const std::vector<float>& points,
    const std::vector<std::uint32_t>& indices)
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
        int node_index = static_cast<int>(nodes.size());
        nodes.push_back(node);
        int count = end - start;
        if (count == 1)
        {
            node.first_triangle = tri_indices[start];
            node.triangle_count = 1;
            nodes[node_index] = node;
            return node_index;
        }
        AABB centroid_bounds;
        for (int i = start; i < end; ++i)
            centroid_bounds.expand(tris[tri_indices[i]].centroid);
        glm::vec3 extent = centroid_bounds.max - centroid_bounds.min;
        int axis = 0;
        if (extent.y > extent.x)
            axis = 1;
        if (extent.z > extent[axis])
            axis = 2;
        int mid = start + count / 2;
        std::nth_element(
            tri_indices.begin() + start,
            tri_indices.begin() + mid,
            tri_indices.begin() + end,
            [&](int a, int b) {
                return tris[a].centroid[axis] < tris[b].centroid[axis];
            });
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

