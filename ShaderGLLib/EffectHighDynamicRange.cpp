#include "EffectHighDynamicRange.h"
#include <assert.h>

namespace sgl {

	EffectHighDynamicRange::EffectHighDynamicRange(
		const std::shared_ptr<Texture>& out_texture, 
		const std::shared_ptr<Texture>& in_texture, 
		const float exposure /*= 1.0f*/, 
		const float gamma /*= 1.0f*/) :
		exposure_(exposure),
		gamma_(gamma)
	{
		in_material_.AddTexture("Image", in_texture);
		out_material_.AddTexture("FragOut", out_texture);
	}

	void EffectHighDynamicRange::Startup(
		std::pair<std::uint32_t, std::uint32_t> size)
	{
		size_ = size;
		frame_.AttachRender(render_);
		render_.CreateStorage(size_);
		program_ = Program::CreateProgram("HighDynamicRange");
		program_->Use();
		program_->UniformFloat("exposure", exposure_);
		program_->UniformFloat("gamma", gamma_);
		quad_ = CreateQuadMesh(program_);
	}

	void EffectHighDynamicRange::Draw(const double dt /*= 0.0*/)
	{
		ScopedBind scoped_frame(frame_);

		// Set the view port for rendering.
		glViewport(0, 0, size_.first, size_.second);
		error_.Display(__FILE__, __LINE__ - 1);

		// Clear the screen.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error_.Display(__FILE__, __LINE__ - 1);

		frame_.AttachTexture(*out_material_.GetTexture("FragOut"));
		frame_.DrawBuffers(1);
		
		quad_->SetMaterial(in_material_);
		quad_->Draw();
	}

} // End namespace sgl.