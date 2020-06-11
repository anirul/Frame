#pragma once

#include "../ShaderGLLib/Window.h"
#include "../ShaderGLLib/Device.h"

class Input : public sgl::InputInterface
{
public:
	Input(const std::shared_ptr<sgl::Device>& device) : device_(device) {}

public:
	void KeyPressed(const char key) override;
	void KeyReleased(const char key) override;
	void MouseMoved(
		const glm::vec2 position,
		const glm::vec2 relative) override;
	void MousePressed(const char button) override;
	void MouseReleased(const char button) override;

private:
	std::shared_ptr<sgl::Device> device_;
};