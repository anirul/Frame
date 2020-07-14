#include "EffectScreenSpaceAmbientOcclusion.h"
#include <assert.h>
#include <random>

namespace sgl {

	namespace
	{
		constexpr float lerp(float a, float b, float t) noexcept
		{
			return a + t * (b - a);
		}
	}


	EffectScreenSpaceAmbientOcclusion::EffectScreenSpaceAmbientOcclusion(
		const std::shared_ptr<Texture>& out_texture, 
		const std::vector<std::shared_ptr<Texture>>& in_textures,
		const glm::mat4& projection) :
		projection_(projection)
	{
		assert(in_textures.size() == 2);
		out_material_.AddTexture("FragOut", out_texture);

		// Noise and kernel.
		std::default_random_engine generator;
		std::uniform_real_distribution<GLfloat> random_length(-1.0, 1.0);
		std::uniform_real_distribution<GLfloat> random_unit(0.0, 1.0);
		for (unsigned int i = 0; i < 64; ++i)
		{
			glm::vec3 sample(
				random_length(generator),
				random_length(generator),
				random_unit(generator));
			sample = glm::normalize(sample);
			sample *= random_unit(generator);
			float scale = static_cast<float>(i) / 64.0f;
			// Scale sample s.t. they are more aligned to the center of the
			// kernel
			scale = lerp(0.1f, 1.0f, scale * scale);
			sample *= scale;
			kernel_ssao_vec_.emplace_back(sample);
		}
		std::vector<glm::vec3> ssao_noise = {};
		for (unsigned int i = 0; i < 16; ++i)
		{
			ssao_noise.emplace_back(
				glm::vec3(
					random_length(generator),
					random_length(generator),
					0.0f));
		}
		auto noise_texture = std::make_shared<Texture>(
			std::pair{ 4, 4 },
			ssao_noise.data(),
			sgl::PixelElementSize::HALF,
			sgl::PixelStructure::GREY_ALPHA);
		noise_texture->SetWrapS(TextureFilter::REPEAT);
		noise_texture->SetWrapT(TextureFilter::REPEAT);

		// Add texture to the material.
		in_material_.AddTexture("ViewPosition", in_textures[0]);
		in_material_.AddTexture("ViewNormal", in_textures[1]);
		in_material_.AddTexture("Noise", noise_texture);
	}

	void EffectScreenSpaceAmbientOcclusion::Startup(
		std::pair<std::uint32_t, std::uint32_t> size)
	{
		size_ = size;
		frame_.AttachRender(render_);
		render_.CreateStorage(size_);
		program_ = Program::CreateProgram("ScreenSpaceAmbientOcclusion");
		program_->Use();
		for (int i = 0; i < 64; ++i)
		{
			program_->UniformVector3(
				"kernel[" + std::to_string(i) + "]", 
				kernel_ssao_vec_[i]);
		}
		program_->UniformVector2(
			"noise_scale", 
			glm::vec2(
				static_cast<float>(size.first) / 4.0f,
				static_cast<float>(size.second) / 4.0f));
		program_->UniformMatrix("projection", projection_);
		quad_ = CreateQuadMesh(program_);
		frame_.AttachTexture(*out_material_.GetTexture("FragOut"));
		frame_.DrawBuffers(1);
	}

	void EffectScreenSpaceAmbientOcclusion::Draw(const double dt /*= 0.0*/)
	{
		ScopedBind scoped_frame(frame_);

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
