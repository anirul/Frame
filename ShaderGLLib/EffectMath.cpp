#include "EffectMath.h"
#include <assert.h>

namespace sgl {

	EffectAddition::EffectAddition(
		const std::shared_ptr<Texture>& out_texture,
		const std::vector<std::shared_ptr<Texture>>& in_textures)
	{
		for (int i = 0; i < in_textures.size(); ++i)
		{
			in_material_.AddTexture(
				"Texture" + std::to_string(i),
				in_textures[i]);
		}
		out_material_.AddTexture("FragOut", out_texture);
	}

	void EffectAddition::Startup(std::pair<std::uint32_t, std::uint32_t> size)
	{
		assert(in_material_.GetMap().size() >= 2);
		size_ = size;
		frame_.AttachRender(render_);
		render_.CreateStorage(size_);
		program_ = Program::CreateProgram("VectorAddition");
		program_->Use();
		quad_ = CreateQuadMesh(program_);
	}

	void EffectAddition::Draw(const double dt /*= 0.0*/)
	{
		assert(in_material_.GetMap().size() >= 2);

		ScopedBind scoped_frame(frame_);
		ScopedBind scoped_render(render_);

		glViewport(0, 0, size_.first, size_.second);
		error_.Display(__FILE__, __LINE__ - 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error_.Display(__FILE__, __LINE__ - 1);

		frame_.AttachTexture(*out_material_.GetMap().begin()->second);
		frame_.DrawBuffers(1);
		program_->Use();
		program_->UniformInt(
			"texture_max",
			static_cast<int>(in_material_.GetMap().size()));
		quad_->SetMaterial(in_material_);
		quad_->Draw();
	}

	EffectMultiply::EffectMultiply(
		const std::shared_ptr<Texture>& out_texture,
		const std::vector<std::shared_ptr<Texture>>& in_textures)
	{
		for (int i = 0; i < in_textures.size(); ++i)
		{
			in_material_.AddTexture(
				"Texture" + std::to_string(i),
				in_textures[i]);
		}
		out_material_.AddTexture("FragOut", out_texture);
	}

	void EffectMultiply::Startup(std::pair<std::uint32_t, std::uint32_t> size)
	{
		assert(in_material_.GetMap().size() >= 2);
		size_ = size;
		frame_.AttachRender(render_);
		render_.CreateStorage(size_);
		program_ = Program::CreateProgram("VectorMultiply");
		program_->Use();
		quad_ = CreateQuadMesh(program_);
	}

	void EffectMultiply::Draw(const double dt /*= 0.0*/)
	{
		assert(in_material_.GetMap().size() >= 2);

		ScopedBind scoped_frame(frame_);
		ScopedBind scoped_render(render_);

		glViewport(0, 0, size_.first, size_.second);
		error_.Display(__FILE__, __LINE__ - 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error_.Display(__FILE__, __LINE__ - 1);

		frame_.AttachTexture(*out_material_.GetMap().begin()->second);
		frame_.DrawBuffers(1);
		program_->Use();
		program_->UniformInt(
			"texture_max",
			static_cast<int>(in_material_.GetMap().size()));
		quad_->SetMaterial(in_material_);
		quad_->Draw();
	}

} // End namespace sgl.
