#include "Draw.h"

void Draw::Initialize(const std::pair<std::uint32_t, std::uint32_t> size) 
{
	out_textures_.emplace_back(std::make_shared<sgl::Texture>(size));
}

const std::shared_ptr<sgl::Texture>& Draw::GetDrawTexture() const
{
	return out_textures_[0];
}

void Draw::RunDraw(const double dt)
{
	float dtf = static_cast<float>(dt);
	auto device = window_->GetUniqueDevice();
	glm::vec4 position = { 0.f, 0.f, 2.f, 1.f };
	glm::mat4 rot_y(1.0f);
	rot_y = glm::rotate(rot_y, dtf * -.1f, glm::vec3(0.f, 1.f, 0.f));
	sgl::Camera cam(glm::vec3(position * rot_y), { 0.f, 0.f, 0.f });
	device->SetCamera(cam);
	device->DrawMultiTextures(out_textures_, dt);
}

void Draw::Delete() {}
