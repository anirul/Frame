#pragma once

#include <memory>

#include "Frame/OpenGL/Texture.h"
#include "Frame/Window.h"

class Application {
   public:
    Application(std::unique_ptr<frame::WindowInterface>&& window);
    void Startup();
    void Run();

   private:
    std::unique_ptr<frame::WindowInterface> window_ = nullptr;
};
