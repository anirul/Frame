#include "Input.h"

bool Input::KeyPressed(const char key, const double dt)
{
	const float inc = 5.f * static_cast<float>(dt);
	auto* camera = device_->GetLevel()->GetDefaultCamera();
	auto position = camera->GetPosition();
	auto right = camera->GetRight();
	auto up = camera->GetUp();
	auto front = camera->GetFront();
	if (key == 'a')
	{
		position -= right * inc;
	}
	if (key == 'd')
	{
		position += right * inc;
	}
	if (key == 'w')
	{
		position += front * inc;
	}
	if (key == 's')
	{
		position -= front * inc;
	}
	camera->SetPosition(position);
	return true;
}

bool Input::KeyReleased(const char key, const double dt)
{
	return true;
}

bool Input::MouseMoved(
	const glm::vec2 position, 
	const glm::vec2 relative, 
	const double dt)
{
	const float inc = 1.f * static_cast<float>(dt);
	auto* camera = device_->GetLevel()->GetDefaultCamera();
	auto front = camera->GetFront();
	auto right = camera->GetRight();
	auto up = camera->GetUp();
	if (relative.x) 
	{
		front += right * relative.x * inc;
	}
	if (relative.y)
	{
		front -= up * relative.y * inc;
	}
	camera->SetFront(front);
	camera->SetUp({ 0, 1, 0 });
	return true;
}

bool Input::MousePressed(const char button, const double dt)
{
	return true;
}

bool Input::MouseReleased(const char button, const double dt)
{
	return true;
}

void Input::NextFrame()
{
	throw std::runtime_error("Not implemented yet!");
}
