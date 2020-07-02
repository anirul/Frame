#include "Fill.h"
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
		const std::shared_ptr<Texture>& in_texture)
	{
		in_material_.AddTexture("Image", in_texture);
		out_material_.AddTexture("FragOut", out_texture);
	}

	void EffectGaussianBlur::Startup(
		std::pair<std::uint32_t, std::uint32_t> size)
	{
		size_ = size;
		ScopedBind scoped_render(render_);
		frame_[0].AttachRender(render_);
		frame_[1].AttachRender(render_);
		temp_textures_[0] = 
			std::make_shared<Texture>(size_, sgl::PixelElementSize::HALF);
		temp_textures_[1] =
			std::make_shared<Texture>(size_, sgl::PixelElementSize::HALF);
		frame_[0].AttachTexture(*temp_textures_[0]);
		frame_[1].AttachTexture(*temp_textures_[1]);
		render_.CreateStorage(size_);
		program_ = Program::CreateProgram("GaussianBlur");
		quad_ = CreateQuadMesh(program_);
	}

	void EffectGaussianBlur::Draw(const double dt /*= 0.0*/)
	{
		ScopedBind scoped_render(render_);

		// Set the view port for rendering.
		glViewport(0, 0, size_.first, size_.second);
		error_.Display(__FILE__, __LINE__ - 1);

		// Clear the screen.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error_.Display(__FILE__, __LINE__ - 1);

		program_->Use();

		bool horizontal = true;
		bool first_iteration = true;
		for (int i = 0; i < 10; ++i)
		{
			// Reset the texture manager.
			auto material = std::make_shared<Material>();
			program_->UniformInt("horizontal", horizontal);
			ScopedBind scoped_frame(frame_[horizontal]);
			material->AddTexture(
				"Image",
				(first_iteration) ? 
					in_material_.GetMap().begin()->second : 
					temp_textures_[!horizontal]);
			quad_->SetMaterial(material);
			quad_->Draw();
			horizontal = !horizontal;
			if (first_iteration) first_iteration = false;
		}

		out_material_.AddTexture("FragOut", temp_textures_[!horizontal]);
	}

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
