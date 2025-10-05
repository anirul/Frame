#include "frame/common/draw.h"

#include <fstream>

#include "frame/file/file_system.h"
#include "frame/json/level_data.h"
#include "frame/json/parse_level.h"
#include "frame/vulkan/device.h"
#include "frame/logger.h"

namespace frame::common
{

// CHECKME(anirul): Why not assign size to size_?
void Draw::Startup(glm::uvec2 size)
{
    size_ = size;
    if (draw_type_based_ == DrawTypeEnum::PATH)
    {
        const auto backend = device_.GetDeviceEnum();
        if (backend == frame::RenderingAPIEnum::OPENGL)
        {
            level_ = frame::json::ParseLevel(size_, path_);
        }
        else if (backend == frame::RenderingAPIEnum::VULKAN)
        {
            const auto level_proto = frame::json::LoadLevelProto(path_);
            const auto asset_root = frame::file::FindDirectory("asset");
            const auto level_data =
                frame::json::BuildLevelData(size_, level_proto, asset_root);
            auto* vulkan_device = dynamic_cast<frame::vulkan::Device*>(&device_);
            if (!vulkan_device)
            {
                throw std::runtime_error("Vulkan device not available.");
            }
            vulkan_device->StartupFromLevelData(level_data);
            return;
        }
        else
        {
            frame::Logger::GetInstance()->error(
                "Unsupported rendering backend {} for JSON level loading.",
                static_cast<int>(backend));
            throw std::runtime_error("Unsupported rendering backend.");
        }
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
    StaticMeshInterface& static_mesh,
    MaterialInterface& material)
{
}

} // End namespace frame::common.
