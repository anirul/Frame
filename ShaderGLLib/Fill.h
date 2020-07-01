#pragma once

#include "../ShaderGLLib/Device.h"

namespace sgl {

	class EffectBrightness : public EffectInterface
	{
	public:
		EffectBrightness(
			const std::shared_ptr<Texture>& out_texture,
			const std::shared_ptr<Texture>& in_texture);
		virtual ~EffectBrightness() { Delete(); }

	public:
		void Startup(std::pair<std::uint32_t, std::uint32_t> size) override;
		void Draw(const double dt = 0.0) override;
		void Delete() override {}

	public:
		const std::string GetName() const override { return "Brightness"; }

	protected:
		const Error& error_ = Error::GetInstance();
		std::pair<std::uint32_t, std::uint32_t> size_ = {0, 0};
		Material out_material_ = {};
		Material in_material_ = {};
		Frame frame_ = {};
		Render render_ = {};
		std::shared_ptr<Program> program_ = nullptr;
		std::shared_ptr<Mesh> quad_ = nullptr;
	};

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
		std::shared_ptr<Material> out_material_ = nullptr;
		std::shared_ptr<Material> in_material_ = nullptr;
		std::shared_ptr<Frame> frame_ = nullptr;
		std::shared_ptr<Render> render_ = nullptr;
		std::shared_ptr<Program> program_ = nullptr;
		std::shared_ptr<Mesh> quad_ = nullptr;
		float exponent_ = 1.0f;
	};

	class EffectGaussianBlur : public EffectInterface
	{
	public:
		EffectGaussianBlur(
			const std::shared_ptr<Texture>& out_texture,
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
		std::shared_ptr<Material> out_material_ = nullptr;
		std::shared_ptr<Material> in_material_ = nullptr;
		std::shared_ptr<Frame> frame_[2] = { nullptr, nullptr };
		std::shared_ptr<Render> render_ = nullptr;
		std::shared_ptr<Program> program_ = nullptr;
		std::shared_ptr<Mesh> quad_ = nullptr;
		std::shared_ptr<Texture> temp_textures_[2] = { nullptr, nullptr };
	};

	class EffectAddition : public EffectInterface
	{
	public:
		EffectAddition(
			const std::shared_ptr<Texture>& out_texture,
			const std::vector<std::shared_ptr<Texture>>& in_textures);
		virtual ~EffectAddition() { Delete(); }

	public:
		void Startup(std::pair<std::uint32_t, std::uint32_t> size) override;
		void Draw(const double dt = 0.0) override;
		void Delete() override {}

	public:
		const std::string GetName() const override { return "Addition"; }

	protected:
		const Error& error_ = Error::GetInstance();
		std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
		std::shared_ptr<Material> out_material_ = nullptr;
		std::shared_ptr<Material> in_material_ = nullptr;
		std::shared_ptr<Frame> frame_ = nullptr;
		std::shared_ptr<Render> render_ = nullptr;
		std::shared_ptr<Program> program_ = nullptr;
		std::shared_ptr<Mesh> quad_ = nullptr;
	};

	class EffectMultiply : public EffectInterface
	{
	public:
		EffectMultiply(
			const std::shared_ptr<Texture>& out_texture,
			const std::vector<std::shared_ptr<Texture>>& in_textures);
		virtual ~EffectMultiply() { Delete(); }

	public:
		void Startup(std::pair<std::uint32_t, std::uint32_t> size) override;
		void Draw(const double dt = 0.0) override;
		void Delete() override {}

	public:
		const std::string GetName() const override { return "Multiply"; }

	protected:
		const Error& error_ = Error::GetInstance();
		std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
		std::shared_ptr<Material> out_material_ = nullptr;
		std::shared_ptr<Material> in_material_ = nullptr;
		std::shared_ptr<Frame> frame_ = nullptr;
		std::shared_ptr<Render> render_ = nullptr;
		std::shared_ptr<Program> program_ = nullptr;
		std::shared_ptr<Mesh> quad_ = nullptr;
	};

} // End namespace sgl.