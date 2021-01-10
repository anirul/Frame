#pragma once

#include "Frame/DeviceInterface.h"
#include "Frame/DrawInterface.h"
#include "Frame/Error.h"
#include "Frame/Logger.h"
#include "Frame/Window.h"
#include "Sample/Common/NameInterface.h"

class Draw : public frame::DrawInterface
{
public:
	Draw(
		const std::pair<std::uint32_t, std::uint32_t> size,
		const std::shared_ptr<NameInterface>& name,
		const std::shared_ptr<frame::DeviceInterface> device) : 
		size_(size),
		name_(name),
		device_(device) {}

public:
	void Startup(const std::pair<std::uint32_t, std::uint32_t> size) override;
	void RunDraw(const double dt) override;

private:
	frame::Error& error_ = frame::Error::GetInstance();
	frame::Logger& logger_ = frame::Logger::GetInstance();
	std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
	std::shared_ptr<NameInterface> name_ = nullptr;
	std::shared_ptr<frame::DeviceInterface> device_ = nullptr;
};