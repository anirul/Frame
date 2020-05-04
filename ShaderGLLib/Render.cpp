#include "Render.h"
#include <stdexcept>
#include <GL/glew.h>

namespace sgl {

	Render::Render()
	{
		glGenRenderbuffers(1, &render_id_);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	Render::~Render()
	{
		glDeleteRenderbuffers(1, &render_id_);
	}

	void Render::Bind() const
	{
		glBindRenderbuffer(GL_RENDERBUFFER, render_id_);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Render::UnBind() const
	{
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		error_.Display(__FILE__, __LINE__ - 1);
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
		error_.Display(__FILE__, __LINE__ - 5);
	}

} // End namespace sgl.
