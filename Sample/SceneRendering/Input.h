#pragma once

#include "../ShaderGLLib/Window.h"
#include "../ShaderGLLib/Device.h"

class Input : public sgl::InputInterface
{
public:
	Input(const std::shared_ptr<sgl::Device>& device) : device_(device) {}

public:
	bool KeyPressed(const char key, const double dt) override;
	bool KeyReleased(const char key, const double dt) override;
	bool MouseMoved(
		const glm::vec2 position,
		const glm::vec2 relative, const double dt) override;
	bool MousePressed(const char button, const double dt) override;
	bool MouseReleased(const char button, const double dt) override;
	int GetValue() const override { return value_; }

private:
	std::shared_ptr<sgl::Device> device_;
	int value_ = 0;
};