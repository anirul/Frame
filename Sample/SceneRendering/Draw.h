#pragma once

#include "Frame/Logger.h"
#include "Frame/OpenGL/Device.h"
#include "Frame/OpenGL/Fill.h"
#include "Frame/Window.h"

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
	std::string preferred_texture_ = "";
	std::map<std::string, std::shared_ptr<frame::TextureInterface>> 
		texture_map_ = {};
	std::map<std::string, std::shared_ptr<frame::ProgramInterface>> 
		program_map_ = {};
};
