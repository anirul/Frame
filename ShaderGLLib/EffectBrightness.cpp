#include "EffectBrightness.h"
#include <assert.h>

namespace sgl {

	EffectBrightness::EffectBrightness(
		const std::shared_ptr<Texture>& out_texture,
		const std::shared_ptr<Texture>& in_texture)
	{
		in_material_.AddTexture("Image", in_texture);
		out_material_.AddTexture("FragOut", out_texture);
	}

	void EffectBrightness::Startup(
		std::pair<std::uint32_t, std::uint32_t> size)
	{
		assert(size == in_material_.GetMap().at("Image")->GetSize());
		assert(size == out_material_.GetMap().at("FragOut")->GetSize());
		size_ = size;
		program_ = Program::CreateProgram("Brightness");
		program_->Use();
		frame_.AttachRender(render_);
		quad_ = CreateQuadMesh(program_);
		render_.CreateStorage(size_);
	}

	void EffectBrightness::Draw(const double dt /*= 0.0*/)
	{
		ScopedBind scoped_frame(frame_);

		frame_.AttachTexture(*out_material_.GetMap().at("FragOut"));
		frame_.DrawBuffers();

		// Set the view port for rendering.
		glViewport(0, 0, size_.first, size_.second);
		error_.Display(__FILE__, __LINE__ - 1);

		// Clear the screen.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error_.Display(__FILE__, __LINE__ - 1);

		quad_->SetMaterial(in_material_);
		quad_->Draw();
	}

} // End namespace sgl.
