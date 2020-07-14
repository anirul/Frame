#pragma once

#include <glm/glm.hpp>
#include "../ShaderGLLib/Texture.h"
#include "../shaderGLLib/Device.h"

namespace sgl {

	class EffectBlur : public EffectInterface
	{
	public:
		EffectBlur(
			const std::shared_ptr<Texture>& out_texture,
			const std::shared_ptr<Texture>& in_texture,
			const float exponent);
		virtual ~EffectBlur() { Delete(); }

	public:
		void Startup(std::pair<std::uint32_t, std::uint32_t> size) override;
		void Draw(const double dt = 0.0) override;
		void Delete() override {}

	public:
		const std::string GetName() const override { return "Blur"; }

	protected:
		const Error& error_ = Error::GetInstance();
		std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
		Material out_material_ = {};
		Material in_material_ = {};
		Frame frame_ = {};
		Render render_ = {};
		std::shared_ptr<Program> program_ = nullptr;
		std::shared_ptr<Mesh> quad_ = nullptr;
		float exponent_ = 1.0f;
	};

	class EffectGaussianBlur : public EffectInterface
	{
	public:
		EffectGaussianBlur(
			const std::shared_ptr<Texture>& out_texture,
			const std::shared_ptr<Texture>& in_texture,
			const bool horizontal);
		virtual ~EffectGaussianBlur() { Delete(); }

	public:
		void Startup(std::pair<std::uint32_t, std::uint32_t> size) override;
		void Draw(const double dt = 0.0) override;
		void Delete() override {}

	public:
		const std::string GetName() const override { return "GaussianBlur"; }

	protected:
		const Error& error_ = Error::GetInstance();
		const bool horizontal_ = true;
		std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
		Material out_material_ = {};
		Material in_material_ = {}; 
		Frame frame_ = {};
		Render render_ = {};
		std::shared_ptr<Program> program_ = nullptr;
		std::shared_ptr<Mesh> quad_ = nullptr;
	};

} // End namespace sgl.
