#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "frame/file/obj.h"

namespace frame::file
{

struct ObjCacheMetadata
{
    std::filesystem::path cache_path;
    std::string cache_relative;
    std::string source_relative;
    std::uint64_t source_size = 0;
    std::uint64_t source_mtime_ns = 0;
};

struct ObjCachePayload
{
    bool hasTextureCoordinates = false;
    std::vector<ObjMesh> meshes;
    std::vector<ObjMaterial> materials;
};

std::optional<ObjCachePayload> LoadObjCache(const ObjCacheMetadata& metadata);
std::optional<ObjCachePayload> LoadObjCacheRelaxed(
    const std::filesystem::path& cache_path);

void SaveObjCache(
    const ObjCacheMetadata& metadata, const ObjCachePayload& payload);

} // namespace frame::file
