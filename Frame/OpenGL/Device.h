#pragma once

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <SDL2/SDL.h>

#include "Frame/Camera.h"
#include "Frame/DeviceInterface.h"
#include "Frame/Logger.h"
#include "Frame/UniformInterface.h"
#include "Frame/OpenGL/Program.h"
#include "Frame/OpenGL/Texture.h"
#include "Frame/OpenGL/Buffer.h"
#include "Frame/OpenGL/StaticMesh.h"
#include "Frame/OpenGL/Light.h"
#include "Frame/OpenGL/Renderer.h"
#include "Frame/OpenGL/Material.h"
#include "Frame/NodeCamera.h"

namespace frame::opengl {

	class Device : public DeviceInterface
	{
	public:
		// This will initialize the GL context and make the GLEW init.
		Device(
			void* gl_context, 
			const std::pair<std::uint32_t, std::uint32_t> size);
		virtual ~Device();

	public:
		// Clear the Screen.
		void Clear(
			const glm::vec4& color = 
				glm::vec4(.2f, 0.f, .2f, 1.0f)) const final;
		// Startup the scene.
		void Startup(std::unique_ptr<LevelInterface>&& level) final;
		// Cleanup the mess.
		void Cleanup() final;
		// Display to the screen.
		void Display(double dt = 0.0) final;
		// Make a screen shot to a file.
		void ScreenShot(const std::string& file) const final;

	public:
		LevelInterface* GetLevel() final { return level_.get(); }
		void* GetDeviceContext() const final { return gl_context_; }
		const std::string GetTypeString() const final { return "OpenGL"; }

	private:
		// Map of current stored level.
		std::unique_ptr<LevelInterface> level_ = nullptr;
		// Open GL context.
		void* gl_context_ = nullptr;
		// Constants.
		const std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
		const proto::PixelElementSize pixel_element_size_ = 
			proto::PixelElementSize_HALF();
		// Rendering pipeline.
		std::unique_ptr<Renderer> renderer_ = nullptr;
		// Error setup.
		const Logger& logger_ = Logger::GetInstance();
	};

} // End namespace frame::opengl.
