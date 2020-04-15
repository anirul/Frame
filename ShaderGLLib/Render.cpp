#include "Render.h"
#include <stdexcept>
#include <gl/glew.h>

namespace sgl {

	Render::Render()
	{
		glGenRenderbuffers(1, &render_id_);
		if (render_id_ == 0)
		{
			throw std::runtime_error("Couldn't generate a render buffer.");
		}
	}

	Render::~Render()
	{
		UnBind();
		glDeleteRenderbuffers(1, &render_id_);
	}

	void Render::Bind() const
	{
		glBindRenderbuffer(GL_RENDERBUFFER, render_id_);
	}

	void Render::UnBind() const
	{
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}

	void Render::BindStorage(
		const std::pair<std::uint32_t, std::uint32_t> size, 
		const PixelDepthComponent pixel_depth_component /*= 
			PixelDepthComponent::DEPTH_COMPONENT24*/) const
	{
		Bind();
		glRenderbufferStorage(
			GL_RENDERBUFFER,
			static_cast<int>(pixel_depth_component),
			size.first,
			size.second);
	}

} // End namespace sgl.
