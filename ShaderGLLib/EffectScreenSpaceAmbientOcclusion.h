#pragma once

#include <glm/glm.hpp>
#include "../ShaderGLLib/Texture.h"
#include "../shaderGLLib/Device.h"

namespace sgl {

	class EffectScreenSpaceAmbientOcclusion : public EffectInterface
	{
	public:
		EffectScreenSpaceAmbientOcclusion(
			const std::shared_ptr<Texture>& out_texture,
			const std::vector<std::shared_ptr<Texture>>& in_textures,
			const glm::mat4& projection);
		virtual ~EffectScreenSpaceAmbientOcclusion() { Delete(); }

	public:
		void Startup(std::pair<std::uint32_t, std::uint32_t> size) override;
		void Draw(const double dt = 0.0) override;
		void Delete() override {}

	public:
		const std::string GetName() const override 
		{ 
			return "ScreenSpaceAmbientOcclusion"; 
		}

	protected:
		const Error& error_ = Error::GetInstance();
		std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
		std::vector<glm::vec3> kernel_ssao_vec_ = {};
		const glm::mat4& projection_;
		Material out_material_ = {};
		Material in_material_ = {};
		Frame frame_ = {};
		Render render_ = {};
		std::shared_ptr<Program> program_ = nullptr;
		std::shared_ptr<Mesh> quad_ = nullptr;
	};

} // End namespace sgl.
