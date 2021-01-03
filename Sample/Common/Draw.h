#pragma once

#include "../Frame/DeviceInterface.h"
#include "../Frame/DrawInterface.h"
#include "../Frame/Error.h"
#include "../Frame/Logger.h"
#include "../Frame/Window.h"

class Draw : public frame::DrawInterface
{
public:
	Draw(const std::shared_ptr<frame::DeviceInterface> device) : 
		device_(device) {}

public:
	void Startup(const std::pair<std::uint32_t, std::uint32_t> size) override;
	void RunDraw(const double dt) override;

private:
	frame::Error& error_ = frame::Error::GetInstance();
	frame::Logger& logger_ = frame::Logger::GetInstance();
	std::shared_ptr<frame::DeviceInterface> device_ = nullptr;
};