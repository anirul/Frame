#include "frame/common/draw.h"

#include <fstream>

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"

namespace frame::common {

void Draw::Startup(const std::pair<std::uint32_t, std::uint32_t> size) {
    // Just one case the other one is treated after.
    if (draw_type_based_ == DrawTypeEnum::PATH) {
        // Load level from proto files.
        level_ = frame::proto::ParseLevel(size_, path_, device_);
    }
    if (!level_) throw std::runtime_error("No level?");
    device_->Startup(std::move(level_));
    level_ = nullptr;
}

bool Draw::RunDraw(const double dt) { return true; }

}  // End namespace frame::common.
