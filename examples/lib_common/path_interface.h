#pragma once

#include <string>

#include "frame/file/file_system.h"

struct PathInterface {
    virtual const std::string GetGlobalPath() const {
        return frame::file::FindDirectory("asset/json/");
    }
    virtual const std::string GetLevelFileName() const = 0;
};
