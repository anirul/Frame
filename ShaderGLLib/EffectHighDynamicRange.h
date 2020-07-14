#pragma once

#include <glm/glm.hpp>
#include "../ShaderGLLib/Texture.h"
#include "../shaderGLLib/Device.h"

namespace sgl {

	class EffectHighDynamicRange : public EffectInterface
	{
	public:
		EffectHighDynamicRange(
			const std::shared_ptr<Texture>& out_texture,
			const std::shared_ptr<Texture>& in_texture,
			const float exposure = 1.0f,
			const float gamma = 1.0f);
		virtual ~EffectHighDynamicRange() { Delete(); }

	public:
		void Startup(std::pair<std::uint32_t, std::uint32_t> size) override;
		void Draw(const double dt = 0.0) override;
		void Delete() override {}

	public:
		const std::string GetName() const override { return "HighDyamicRange"; }

	protected:
		const Error& error_ = Error::GetInstance();
		std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
		Material out_material_ = {};
		Material in_material_ = {};
		Frame frame_ = {};
		Render render_ = {};
		const float exposure_;
		const float gamma_;
		std::shared_ptr<Program> program_ = nullptr;
		std::shared_ptr<Mesh> quad_ = nullptr;
	};

} // End namespace sgl.
