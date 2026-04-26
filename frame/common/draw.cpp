#include "frame/common/draw.h"

#include <stdexcept>

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"
#include "frame/level_data_startup_internal.h"

namespace frame::common
{

// CHECKME(anirul): Why not assign size to size_?
void Draw::Startup(glm::uvec2 size)
{
    size_ = size;
    if (draw_type_based_ == DrawTypeEnum::PATH)
    {
        const auto asset_root = frame::file::FindDirectory("asset");
        const auto level_data =
            frame::json::ParseLevelData(size_, path_, asset_root);
        auto* level_data_startup =
            dynamic_cast<frame::internal::LevelDataStartupInterface*>(
                &device_);
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


