#pragma once

#include <string>

#include "frame/file/file_system.h"

namespace frame::common {

struct PathInterface {
  virtual const std::filesystem::path GetGlobalPath() const {
    return frame::file::FindDirectory("asset/json/");
  }
  virtual const std::filesystem::path GetLevelFileName() const = 0;
};

}  // End namespace frame::common.
