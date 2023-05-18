#include "frame/common/draw.h"

#include <fstream>

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"

namespace frame::common {

// CHECKME(anirul): Why not assign size to size_?
void Draw::Startup(const glm::uvec2 size) {
  // Just one case the other one is treated after.
  if (draw_type_based_ == DrawTypeEnum::PATH) {
    // Load level from proto files.
    level_ = frame::proto::ParseLevel(size_, path_);
  }
  if (!level_) throw std::runtime_error("No level?");
  device_.Startup(std::move(level_));
  level_ = nullptr;
}

bool Draw::Update(DeviceInterface& device, double dt) { return true; }

void Draw::PreRender(UniformInterface& uniform, DeviceInterface& device,
                     StaticMeshInterface& static_mesh,
                     MaterialInterface& material) {}

}  // End namespace frame::common.
