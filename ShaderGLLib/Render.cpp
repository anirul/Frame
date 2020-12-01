#include "Render.h"
#include <stdexcept>
#include <GL/glew.h>

namespace sgl {

	Render::Render()
	{
		glGenRenderbuffers(1, &render_id_);
		error_.Display(__FILE__, __LINE__ - 1);
#ifdef _DEBUG
		logger_->info("create a RENDER_BUFFER {}", render_id_);
#endif // _DEBUG
	}

	Render::~Render()
	{
#ifdef _DEBUG
		logger_->info("deleted a RENDER_BUFFER {}", render_id_);
#endif // _DEBUG
		glDeleteRenderbuffers(1, &render_id_);
	}

	void Render::Bind(const unsigned int slot /*= 0*/) const
	{
		assert(slot == 0);
		if (locked_bind_) return;
		glBindRenderbuffer(GL_RENDERBUFFER, render_id_);
		error_.Display(__FILE__, __LINE__ - 1);
#ifdef _DEBUG
		logger_->info("Binded RENDER_BUFFER {}   RB VVVV", render_id_);
#endif
	}

	void Render::UnBind() const
	{
		if (locked_bind_) return;
#ifdef _DEBUG
		logger_->info("UnBinded RENDER_BUFFER {} RB ^^^^", render_id_);
#endif
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Render::CreateStorage(
		const std::pair<std::uint32_t, std::uint32_t> size, 
		const PixelDepthComponent pixel_depth_component /*= 
			PixelDepthComponent::DEPTH_COMPONENT24*/) const
	{
		Bind();
#ifdef _DEBUG
		logger_->info(
			"Create storage in RENDER_BUFFER({}), size ({}, {})",
			render_id_,
			size.first,
			size.second);
#endif // _DEBUG
		glRenderbufferStorage(
			GL_RENDERBUFFER,
			GL_DEPTH24_STENCIL8,
//			static_cast<int>(pixel_depth_component.value()),
			size.first,
			size.second);
		error_.Display(__FILE__, __LINE__ - 5);
		UnBind();
	}

} // End namespace sgl.
