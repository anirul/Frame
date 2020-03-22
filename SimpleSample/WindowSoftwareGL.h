#pragma once

#include "WindowInterface.h"

class WindowSoftwareGL : public SoftwareGL::WindowInterface
{
public:
	WindowSoftwareGL(int width, int height) : 
		width_(width), 
		height_(height) {}
	bool Startup(const std::pair<int, int>& gl_version) override;
	bool RunCompute(const double delta_time) override;
	bool RunEvent(const SDL_Event& event) override;
	void Cleanup() override {}
	const std::pair<int, int> GetWindowSize() const override;

protected:
	int width_ = 640;
	int height_ = 480;
	float z_min_ = 0.1f;
	float z_max_ = 10000.0f;
};
