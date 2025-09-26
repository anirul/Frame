#include "frame/bvh_cache.h"

#include <fstream>
#include <format>

#include "frame/logger.h"
#include "frame/proto/bvh_cache.pb.h"

namespace frame
{

namespace
{

constexpr std::uint32_t kCacheVersion = 1;

Logger& GetLogger()
{
	return Logger::GetInstance();
}

} // namespace

std::optional<std::vector<BVHNode>> LoadBvhCache(const BvhCacheMetadata& metadata)
{
	if (metadata.cache_path.empty())
	{
		return std::nullopt;
	}
	if (!std::filesystem::exists(metadata.cache_path))
	{
		return std::nullopt;
	}
	std::ifstream input(metadata.cache_path, std::ios::binary);
	if (!input)
	{
		GetLogger()->warn(
		    "Failed to open BVH cache file {} for reading.",
		    metadata.cache_path.string());
		return std::nullopt;
	}
	proto::BvhCache cache_proto;
	if (!cache_proto.ParseFromIstream(&input))
	{
		GetLogger()->warn(
		    "Could not parse BVH cache {}.", metadata.cache_path.string());
		return std::nullopt;
	}
	if (cache_proto.cache_version() != kCacheVersion)
	{
		GetLogger()->info(
		    "Ignoring BVH cache {} due to version mismatch ({} != {}).",
		    metadata.cache_path.string(),
		    cache_proto.cache_version(),
		    kCacheVersion);
		return std::nullopt;
	}
	if (cache_proto.source_size() != metadata.source_size ||
	    cache_proto.source_mtime_ns() != metadata.source_mtime_ns ||
	    cache_proto.source_relative() != metadata.source_relative ||
	    cache_proto.cache_relative() != metadata.cache_relative)
	{
		GetLogger()->info(
		    "Ignoring BVH cache {} due to stale source metadata.",
		    metadata.cache_path.string());
		return std::nullopt;
	}
	std::vector<BVHNode> nodes;
	nodes.reserve(cache_proto.nodes_size());
	for (const auto& proto_node : cache_proto.nodes())
	{
		BVHNode node;
		node.min = {proto_node.min_x(), proto_node.min_y(), proto_node.min_z()};
		node.max = {proto_node.max_x(), proto_node.max_y(), proto_node.max_z()};
		node.left = proto_node.left();
		node.right = proto_node.right();
		node.first_triangle = proto_node.first_triangle();
		node.triangle_count = proto_node.triangle_count();
		nodes.push_back(node);
	}
	return nodes;
}

void SaveBvhCache(const BvhCacheMetadata& metadata, const std::vector<BVHNode>& nodes)
{
	if (metadata.cache_path.empty())
	{
		return;
	}
	std::error_code ec;
	const auto parent = metadata.cache_path.parent_path();
	if (!parent.empty())
	{
		std::filesystem::create_directories(parent, ec);
		if (ec)
		{
			GetLogger()->warn(
			    "Failed to create BVH cache directory {}: {}",
			    parent.string(),
			    ec.message());
			return;
		}
	}
	proto::BvhCache cache_proto;
	cache_proto.set_cache_version(kCacheVersion);
	cache_proto.set_cache_relative(metadata.cache_relative);
	cache_proto.set_source_relative(metadata.source_relative);
	cache_proto.set_source_size(metadata.source_size);
	cache_proto.set_source_mtime_ns(metadata.source_mtime_ns);
	for (const auto& node : nodes)
	{
		auto* proto_node = cache_proto.add_nodes();
		proto_node->set_min_x(node.min.x);
		proto_node->set_min_y(node.min.y);
		proto_node->set_min_z(node.min.z);
		proto_node->set_max_x(node.max.x);
		proto_node->set_max_y(node.max.y);
		proto_node->set_max_z(node.max.z);
		proto_node->set_left(node.left);
		proto_node->set_right(node.right);
		proto_node->set_first_triangle(node.first_triangle);
		proto_node->set_triangle_count(node.triangle_count);
	}
	std::ofstream output(metadata.cache_path, std::ios::binary | std::ios::trunc);
	if (!output)
	{
		GetLogger()->warn(
		    "Failed to open BVH cache file {} for writing.",
		    metadata.cache_path.string());
		return;
	}
	if (!cache_proto.SerializeToOstream(&output))
	{
		GetLogger()->warn(
		    "Failed to serialize BVH cache {}.", metadata.cache_path.string());
	}
}

} // namespace frame
