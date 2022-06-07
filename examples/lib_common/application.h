#pragma once

#include <filesystem>
#include <memory>

#include "frame/window.h"
#include "examples/lib_common/path_interface.h"

class Application {
   public:
    Application(std::filesystem::path path, std::unique_ptr<frame::WindowInterface>&& window);
    void Startup();
    void Run();

   protected:
    std::filesystem::path path_;
    std::unique_ptr<frame::WindowInterface> window_ = nullptr;
};
