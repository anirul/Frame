#include "frame/common/draw.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <stdexcept>

#include "absl/flags/flag.h"

#include "frame/common/application.h"
#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"
#include "frame/level_data_startup_internal.h"

namespace frame::common
{

namespace
{

std::string ToLowerAscii(std::string value)
{
    std::transform(
        value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
    return value;
}

std::optional<frame::json::RenderPreset> ParseRenderingOverride(
    const std::string& value)
{
    const auto lowered = ToLowerAscii(value);
    if (lowered == "auto")
    {
        return std::nullopt;
    }
    if (lowered == "raytrace" || lowered == "raytracing")
    {
        return frame::json::RenderPreset::Raytrace;
    }
    if (lowered == "rasterise" || lowered == "rasterising" ||
        lowered == "rasterize" || lowered == "rasterizing" ||
        lowered == "raster")
    {
        return frame::json::RenderPreset::Raster;
    }
    throw std::invalid_argument(
        "Unknown rendering mode '" + value +
        "'. Expected auto, raytrace, raytracing, rasterise, rasterising, "
        "rasterize, rasterizing, or raster.");
}

} // namespace

// CHECKME(anirul): Why not assign size to size_?
void Draw::Startup(glm::uvec2 size)
{
    size_ = size;
    if (draw_type_based_ == DrawTypeEnum::PATH)
    {
        const auto asset_root = frame::file::FindDirectory("asset");
        auto proto_level = frame::json::LoadLevelProto(path_);
        frame::json::LevelDataOptions options;
        if (const auto override_preset =
                ParseRenderingOverride(absl::GetFlag(FLAGS_rendering)))
        {
            options.render_preset = *override_preset;
        }
        const auto level_data = frame::json::ParseLevelData(
            size_, proto_level, asset_root, path_, options);
        auto* level_data_startup =
            dynamic_cast<frame::internal::LevelDataStartupInterface*>(&device_);
        if (!level_data_startup)
        {
            throw std::runtime_error(
                "Device does not support JSON level loading.");
        }
        level_data_startup->StartupFromLevelData(level_data);
        return;
    }
    if (!level_)
    {
        throw std::runtime_error("No level?");
    }
    device_.Startup(std::move(level_));
    level_ = nullptr;
}

bool Draw::Update(DeviceInterface& device, double dt)
{
    return true;
}

void Draw::PreRender(
    UniformCollectionInterface& uniform_collection_interface,
    DeviceInterface& device,
    MeshInterface& mesh,
    MaterialInterface& material)
{
}

} // End namespace frame::common.
