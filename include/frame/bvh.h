#pragma once

#include <cstdint>
#include <vector>
#include <limits>

#include <glm/glm.hpp>

namespace frame
{

struct AABB
{
    glm::vec3 min{std::numeric_limits<float>::max()};
    glm::vec3 max{std::numeric_limits<float>::lowest()};
    void expand(const glm::vec3& p)
    {
        min = glm::min(min, p);
        max = glm::max(max, p);
    }
    void expand(const AABB& b)
    {
        expand(b.min);
        expand(b.max);
    }
};

struct BVHNode
{
    glm::vec3 min;
    float pad0{0.f};
    glm::vec3 max;
    float pad1{0.f};
    int left{-1};
    int right{-1};
    int first_triangle{-1};
    int triangle_count{0};
};

std::vector<BVHNode> BuildBVH(
    const std::vector<float>& points,
    const std::vector<std::uint32_t>& indices);

} // namespace frame

