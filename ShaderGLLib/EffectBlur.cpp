#include "EffectBlur.h"
#include <assert.h>

namespace sgl {

	EffectBlur::EffectBlur(
		const std::shared_ptr<Texture>& out_texture,
		const std::shared_ptr<Texture>& in_texture,
		const float exponent) :
		exponent_(exponent)
	{
		in_material_.AddTexture("Image", in_texture);
		out_material_.AddTexture("FragOut", out_texture);
	}

	void EffectBlur::Startup(std::pair<std::uint32_t, std::uint32_t> size)
	{
		size_ = size;
		ScopedBind scoped_frame(frame_);
		ScopedBind scoped_render(render_);
		frame_.AttachRender(render_);
		render_.CreateStorage(size_);
		program_ = Program::CreateProgram("Blur");
		program_->Use();
		program_->UniformFloat("exponent", exponent_);
		quad_ = CreateQuadMesh(program_);
	}

	void EffectBlur::Draw(const double dt /*= 0.0*/)
	{
		ScopedBind scoped_frame(frame_);
		ScopedBind scoped_render(render_);

		glViewport(0, 0, size_.first, size_.second);
		error_.Display(__FILE__, __LINE__ - 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error_.Display(__FILE__, __LINE__ - 1);

		frame_.AttachTexture(*out_material_.GetMap().at("FragOut"));
		frame_.DrawBuffers(1);
		quad_->SetMaterial(in_material_);
		quad_->Draw();
	}

	EffectGaussianBlur::EffectGaussianBlur(
		const std::shared_ptr<Texture>& out_texture,
		const std::shared_ptr<Texture>& in_texture,
		const bool horizontal) :
		horizontal_(horizontal)
	{
		in_material_.AddTexture("Image", in_texture);
		out_material_.AddTexture("FragOut", out_texture);
	}

	void EffectGaussianBlur::Startup(
		std::pair<std::uint32_t, std::uint32_t> size)
	{
		size_ = size;
		ScopedBind scoped_frame(frame_);
		ScopedBind scoped_render(render_);
		frame_.AttachRender(render_);
		render_.CreateStorage(size_);
		program_ = Program::CreateProgram("GaussianBlur");
		program_->Use();
		program_->UniformInt("horizontal", horizontal_);
		quad_ = CreateQuadMesh(program_);
	}

	void EffectGaussianBlur::Draw(const double dt /*= 0.0*/)
	{
		ScopedBind scoped_frame(frame_);

		// Set the view port for rendering.
		glViewport(0, 0, size_.first, size_.second);
		error_.Display(__FILE__, __LINE__ - 1);
		// Clear the screen.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error_.Display(__FILE__, __LINE__ - 1);

		frame_.AttachTexture(*out_material_.GetMap().at("FragOut"));
		frame_.DrawBuffers(1);
		quad_->SetMaterial(in_material_);
		quad_->Draw();
	}

} // End namespace sgl.
