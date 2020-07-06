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
		std::shared_ptr<Texture> out_texture,
		const std::shared_ptr<Texture>& in_texture) :
		out_texture_(out_texture),
		in_texture_(in_texture) {}

	void EffectGaussianBlur::Startup(
		std::pair<std::uint32_t, std::uint32_t> size)
	{
		assert(in_texture_);
		assert(out_texture_);
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
/*		Material in_material{};
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
		// Go for the swapping (texture temp value).
		// iteration	0	1	2	3	4	5	6	7	8	9
		// horizontal	1	0	1	0	1	0	1	0	1	0
		// frame		1	0	1	0	1	0	1	0	1	0
		// in			in	1	0	1	0	1	0	1	0	1
		// out			1	0	1	0	1	0	1	0	1	out
		for (int i = 0; i < 10; ++i)
		{
			ScopedBind scoped_frame(frame_[horizontal]);
			// Reset the texture manager.
			program_->UniformInt("horizontal", horizontal);
			in_material.AddTexture(
				"Image", 
				(first_iteration) ?
					in_texture_ :
					temp_textures_[!horizontal]);
			frame_[horizontal].AttachTexture(
				(i == 9) ?
					*out_texture_ :
					*temp_textures_[horizontal]);
			frame_[horizontal].DrawBuffers();
			quad_->SetMaterial(in_material);
			quad_->Draw();
			horizontal = !horizontal;
			if (first_iteration) first_iteration = false;
		}
*/

		const sgl::Error& error = sgl::Error::GetInstance();
		sgl::Frame frame[2];
		sgl::Render render{};
		auto size = in_texture_->GetSize();
		frame[0].AttachRender(render);
		frame[1].AttachRender(render);
		render.CreateStorage(size);
		auto pixel_element_size = in_texture_->GetPixelElementSize();

		std::shared_ptr<sgl::Texture> texture_out[2] = {
			std::make_shared<sgl::Texture>(
				size,
				pixel_element_size),
			std::make_shared<sgl::Texture>(
				size,
				pixel_element_size)
		};

		// Set the view port for rendering.
		glViewport(0, 0, size.first, size.second);
		error.Display(__FILE__, __LINE__ - 1);

		// Clear the screen.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error.Display(__FILE__, __LINE__ - 1);

		frame[0].AttachTexture(*texture_out[0]);
		frame[1].AttachTexture(*texture_out[1]);

		static auto program = Program::CreateProgram("GaussianBlur");
		static auto quad = CreateQuadMesh(program);
		program->Use();

		bool horizontal = true;
		bool first_iteration = true;
		for (int i = 0; i < 10; ++i)
		{
			// Reset the texture manager.
			auto material = std::make_shared<Material>();
			program->UniformInt("horizontal", horizontal);
			ScopedBind scoped_frame(frame[horizontal]);
			material->AddTexture(
				"Image",
				(first_iteration) ? in_texture_ : texture_out[!horizontal]);
			quad->SetMaterial(material);
			quad->Draw();
			horizontal = !horizontal;
			if (first_iteration) first_iteration = false;
		}

		out_texture_ = texture_out[!horizontal];
	}

} // End namespace sgl.
