#include "EffectLighting.h"
#include <assert.h>

namespace sgl {

	EffectLighting::EffectLighting(
		const std::shared_ptr<Texture>& out_texture, 
		const std::vector<std::shared_ptr<Texture>>& in_textures,
		const LightManager& light_manager,
		const Camera& camera) :
		light_manager_(light_manager),
		camera_(camera)
	{
		assert(in_textures.size() == 4);
		in_material_.AddTexture("Albedo", in_textures[0]);
		in_material_.AddTexture("Normal", in_textures[1]);
		in_material_.AddTexture("MetalRoughAO", in_textures[2]);
		in_material_.AddTexture("Position", in_textures[3]);
		out_material_.AddTexture("FragOut", out_texture);
	}

	void EffectLighting::Startup(std::pair<std::uint32_t, std::uint32_t> size)
	{
		size_ = size;
		frame_.AttachRender(render_);
		render_.CreateStorage(size_);
		program_ = Program::CreateProgram("Lighting");
		quad_ = CreateQuadMesh(program_);
	}

	void EffectLighting::Draw(const double dt /*= 0.0*/)
	{
		ScopedBind scoped_frame(frame_);

		// Set the view port for rendering.
		glViewport(0, 0, size_.first, size_.second);
		error_.Display(__FILE__, __LINE__ - 1);

		// Clear the screen.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error_.Display(__FILE__, __LINE__ - 1);

		program_->Use();
		program_->UniformVector3(
			"camera_position",
			camera_.GetPosition());
		light_manager_.RegisterToProgram(program_);

		frame_.AttachTexture(*out_material_.GetTexture("FragOut"));
		frame_.DrawBuffers(1);

		quad_->SetMaterial(in_material_);
		quad_->Draw();
	}

} // End namespace sgl.
