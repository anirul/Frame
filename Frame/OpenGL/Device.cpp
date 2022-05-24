#include "Device.h"
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <random>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include "Frame/File/Image.h"
#include "FrameBuffer.h"
#include "RenderBuffer.h"
#include "Fill.h"
#include "Renderer.h"

namespace frame::opengl {

	Device::Device(
		void* gl_context, 
		const std::pair<std::uint32_t, std::uint32_t> size) : 
		gl_context_(gl_context),
		size_(size)
	{
		// Initialize GLEW.
		if (GLEW_OK != glewInit())
		{
			throw std::runtime_error("couldn't initialize GLEW");
		}

		// This should maintain the culling to none.
		// FIXME(anirul): Change this as to be working!
		glDisable(GL_CULL_FACE);
		error_.Display(__FILE__, __LINE__ - 1);
		// glCullFace(GL_BACK);
		// error_.Display(__FILE__, __LINE__ - 1);
		// glFrontFace(GL_CW);
		// error_.Display(__FILE__, __LINE__ - 1);
		// Enable blending to 1 - source alpha.
		glEnable(GL_BLEND);
		error_.Display(__FILE__, __LINE__ - 1);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		error_.Display(__FILE__, __LINE__ - 1);
		glEnable(GL_DEPTH_TEST);
		error_.Display(__FILE__, __LINE__ - 1);
		glDepthFunc(GL_LEQUAL);
		error_.Display(__FILE__, __LINE__ - 1);
		// Enable seamless cube map.
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	Device::~Device() 
	{
		Cleanup();
	}

	void Device::Startup(std::unique_ptr<frame::LevelInterface>&& level)
	{
		// Copy level into the local area.
		level_ = std::move(level);
		// Setup camera.
		auto* camera = level_->GetDefaultCamera();
		if (camera) camera->ComputeView();
		// Create a renderer.
		renderer_ = std::make_unique<Renderer>(level_.get(), size_);
	}

	void Device::Cleanup()
	{
		level_ = nullptr;
		renderer_ = nullptr;
	}

    void Device::Clear(
		const glm::vec4& color/* = glm::vec4(.2f, 0.f, .2f, 1.0f*/) const
    {
        glClearColor(color.r, color.g, color.b, color.a);
        error_.Display(__FILE__, __LINE__ - 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        error_.Display(__FILE__, __LINE__ - 1);
    }

	void Device::Display(double dt /*= 0.0*/)
	{
		if (!renderer_) throw std::runtime_error("No Renderer.");
		renderer_->RenderFromRootNode(dt);
		renderer_->Display(dt);
	}

	void Device::ScreenShot(const std::string& file) const
	{
		auto maybe_texture_id = level_->GetDefaultOutputTextureId();
		if (!maybe_texture_id) throw std::runtime_error("no default texture.");
		auto texture_id = maybe_texture_id.value();
		auto* texture = level_->GetTextureFromId(texture_id);
		if (!texture) throw std::runtime_error("could not open texture.");
		proto::PixelElementSize pixel_element_size{};
		pixel_element_size.set_value(texture->GetPixelElementSize());
		proto::PixelStructure pixel_structure{};
		pixel_structure.set_value(texture->GetPixelStructure());
		file::Image output_image(
			texture->GetSize(),
			pixel_element_size,
			pixel_structure);
		auto vec = texture->GetTextureByte();
		output_image.SetData(vec.data());
		output_image.SaveImageToFile(file);
	}

} // End namespace frame::opengl.
