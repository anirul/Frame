#pragma once

#include "frame/backend_internal.h"
#include "frame/json/level_data.h"

namespace frame::internal
{

class LevelDataStartupInterface
{
  public:
    virtual ~LevelDataStartupInterface() = default;

    virtual void StartupFromLevelData(
        const frame::json::LevelData& level_data) = 0;
};

} // namespace frame::internal
