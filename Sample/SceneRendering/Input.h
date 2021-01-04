#pragma once

#include "../Frame/DeviceInterface.h"
#include "../Frame/Window.h"

class Input : public frame::InputInterface
{
public:
	Input(const std::shared_ptr<frame::DeviceInterface> device) : 
		device_(device) {}

public:
	bool KeyPressed(const char key, const double dt) override;
	bool KeyReleased(const char key, const double dt) override;
	bool MouseMoved(
		const glm::vec2 position,
		const glm::vec2 relative, 
		const double dt) override;
	bool MousePressed(const char button, const double dt) override;
	bool MouseReleased(const char button, const double dt) override;

private:
	std::shared_ptr<frame::DeviceInterface> device_;
};