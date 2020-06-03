#include "Draw.h"

void Draw::Startup(const std::pair<std::uint32_t, std::uint32_t> size)
{
	final_texture_ = std::make_shared<sgl::Texture>(size);
}

const std::shared_ptr<sgl::Texture>& Draw::GetDrawTexture() const
{
	return final_texture_;
}

void Draw::RunDraw(const double dt)
{
	device_->DrawDeferred(dt);
	final_texture_ = device_->DrawLighting();
	final_texture_ = device_->DrawBloom(final_texture_);
	final_texture_ = device_->DrawHighDynamicRange(final_texture_);
}

void Draw::Delete() {}