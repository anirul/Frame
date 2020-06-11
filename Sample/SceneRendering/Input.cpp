#include "Input.h"

void Input::KeyPressed(const char key)
{
	const float inc = 0.1f;
	auto camera = device_->GetCamera();
	auto position = camera.GetPosition();
	auto right = camera.GetRight();
	auto up = camera.GetUp();
	auto front = camera.GetFront();
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
	camera.SetPosition(position);
	device_->SetCamera(camera);
}

void Input::KeyReleased(const char key)
{

}

void Input::MouseMoved(
	const glm::vec2 position,
	const glm::vec2 relative)
{

}

void Input::MousePressed(const char button)
{

}

void Input::MouseReleased(const char button)
{

}
