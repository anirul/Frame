#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "frame/bvh.h"

namespace frame
{

struct BvhCacheMetadata
{
	std::filesystem::path cache_path;
	std::string cache_relative;
	std::string source_relative;
	std::uint64_t source_size = 0;
	std::uint64_t source_mtime_ns = 0;
};

std::optional<std::vector<BVHNode>> LoadBvhCache(const BvhCacheMetadata& metadata);

void SaveBvhCache(const BvhCacheMetadata& metadata, const std::vector<BVHNode>& nodes);

} // namespace frame
