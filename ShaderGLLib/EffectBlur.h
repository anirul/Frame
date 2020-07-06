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
			std::shared_ptr<Texture> out_texture,
			const std::shared_ptr<Texture>& in_texture);
		virtual ~EffectGaussianBlur() { Delete(); }

	public:
		void Startup(std::pair<std::uint32_t, std::uint32_t> size) override;
		void Draw(const double dt = 0.0) override;
		void Delete() override {}

	public:
		const std::string GetName() const override { return "GaussianBlur"; }

	protected:
		const Error& error_ = Error::GetInstance();
		std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
		const std::shared_ptr<Texture>& in_texture_;
		std::shared_ptr<Texture> out_texture_;
		std::shared_ptr<Texture> view_texture_ = nullptr;
		Frame frame_[2] = { {}, {} };
		Render render_ = {};
		std::shared_ptr<Program> program_ = nullptr;
		std::shared_ptr<Mesh> quad_ = nullptr;
		std::shared_ptr<Texture> temp_textures_[2] = { nullptr, nullptr };
	};

} // End namespace sgl.
