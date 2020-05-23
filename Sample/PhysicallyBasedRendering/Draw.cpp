#include "Draw.h"
#include "Application.h"

void Draw::Initialize(const std::pair<std::uint32_t, std::uint32_t> size)
{
	out_textures_.emplace_back(
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize::FLOAT));
	out_textures_.emplace_back(
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize::FLOAT));
}

const std::vector<std::shared_ptr<sgl::Texture>>& Draw::GetTextures()
{
	return out_textures_;
}

void Draw::Run(const double dt)
{
	// Update the camera.
	float dtf = static_cast<float>(dt);
	auto device = window_->GetUniqueDevice();
	glm::vec4 position = { 0.f, 0.f, 2.f, 1.f };
	glm::mat4 rot_y(1.0f);
	rot_y = glm::rotate(rot_y, dtf * -.1f, glm::vec3(0.f, 1.f, 0.f));
	sgl::Camera cam(glm::vec3(position * rot_y), { 0.f, 0.f, 0.f });
	device->SetCamera(cam);
	if (application_->pbr_program_)
	{
		// Don't forget to use before setting any uniform.
		application_->pbr_program_->Use();
		application_->pbr_program_->UniformVector3(
			"camera_position",
			device->GetCamera().GetPosition());
	}
	out_textures_[0] = application_->AddBloom(out_textures_[0]);
}

void Draw::Delete() {}
