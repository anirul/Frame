#include "Draw.h"
#include "../ShaderGLLib/Frame.h"
#include "../ShaderGLLib/Render.h"

void Draw::Startup(const std::pair<std::uint32_t, std::uint32_t> size)
{
	out_textures_.emplace_back(
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize::HALF));
	out_textures_.emplace_back(
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize::HALF));
	out_textures_.emplace_back(
		std::make_shared<sgl::Texture>(size, sgl::PixelElementSize::HALF));
}

const std::shared_ptr<sgl::Texture>& Draw::GetDrawTexture() const
{
	return out_textures_[0];
}

void Draw::RunDraw(const double dt)
{
	// Update the camera.
	float dtf = static_cast<float>(dt);
	glm::vec4 position = { 0.f, 0.f, 2.f, 1.f };
	glm::mat4 rot_y(1.0f);
	rot_y = glm::rotate(rot_y, dtf * -.1f, glm::vec3(0.f, 1.f, 0.f));
	sgl::Camera cam(glm::vec3(position * rot_y), { 0.f, 0.f, 0.f });
	device_->SetCamera(cam);
	if (pbr_program_)
	{
		// Don't forget to use before setting any uniform.
		pbr_program_->Use();
		pbr_program_->UniformVector3(
			"camera_position",
			device_->GetCamera().GetPosition());
	}
	device_->DrawMultiTextures(out_textures_, dt);
	out_textures_[0] = AddBloom(out_textures_[0]);
}

void Draw::Delete() {}

std::shared_ptr<sgl::Texture> Draw::AddBloom(
	const std::shared_ptr<sgl::Texture>& texture) const
{
	auto brightness = CreateBrightness(texture);
	auto gaussian_blur = CreateGaussianBlur(brightness);
	auto merge = MergeDisplayAndGaussianBlur(texture, gaussian_blur);
	return merge;
}

std::shared_ptr<sgl::Texture> Draw::CreateBrightness(
	const std::shared_ptr<sgl::Texture>& texture) const
{
	const sgl::Error& error = sgl::Error::GetInstance();
	sgl::Frame frame{};
	sgl::Render render{};
	auto size = texture->GetSize();
	frame.BindAttach(render);
	render.BindStorage(size);

	auto texture_out = std::make_shared<sgl::Texture>(
		size,
		sgl::PixelElementSize::HALF,
		sgl::PixelStructure::RGB);

	// Set the view port for rendering.
	glViewport(0, 0, size.first, size.second);
	error.Display(__FILE__, __LINE__ - 1);

	// Clear the screen.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	error.Display(__FILE__, __LINE__ - 1);

	frame.BindTexture(*texture_out);

	sgl::TextureManager texture_manager;
	texture_manager.AddTexture("Display", texture);
	auto program = sgl::CreateProgram("Brightness");
	auto quad = sgl::CreateQuadMesh(program);
	quad->SetTextures({ "Display" });
	quad->Draw(texture_manager);

	return texture_out;
}

std::shared_ptr<sgl::Texture> Draw::CreateGaussianBlur(
	const std::shared_ptr<sgl::Texture>& texture) const
{
	const sgl::Error& error = sgl::Error::GetInstance();
	sgl::Frame frame[2];
	sgl::Render render{};
	auto size = texture->GetSize();
	frame[0].BindAttach(render);
	frame[1].BindAttach(render);
	render.BindStorage(size);

	std::shared_ptr<sgl::Texture> texture_out[2] = {
		std::make_shared<sgl::Texture>(
			size,
			sgl::PixelElementSize::HALF,
			sgl::PixelStructure::RGB),
		std::make_shared<sgl::Texture>(
			size,
			sgl::PixelElementSize::HALF,
			sgl::PixelStructure::RGB)
	};

	// Set the view port for rendering.
	glViewport(0, 0, size.first, size.second);
	error.Display(__FILE__, __LINE__ - 1);

	// Clear the screen.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	error.Display(__FILE__, __LINE__ - 1);

	frame[0].BindTexture(*texture_out[0]);
	frame[1].BindTexture(*texture_out[1]);

	auto program = sgl::CreateProgram("GaussianBlur");
	auto quad = sgl::CreateQuadMesh(program);

	bool horizontal = true;
	bool first_iteration = true;
	for (int i = 0; i < 10; ++i)
	{
		// Reset the texture manager.
		sgl::TextureManager texture_manager;
		program->UniformInt("horizontal", horizontal);
		frame[horizontal].Bind();
		texture_manager.AddTexture(
			"Image",
			(first_iteration) ? texture : texture_out[!horizontal]);
		quad->SetTextures({ "Image" });
		quad->Draw(texture_manager);
		horizontal = !horizontal;
		if (first_iteration) first_iteration = false;
	}

	return texture_out[!horizontal];
}

std::shared_ptr<sgl::Texture> Draw::MergeDisplayAndGaussianBlur(
	const std::shared_ptr<sgl::Texture>& display, 
	const std::shared_ptr<sgl::Texture>& gaussian_blur, 
	const float exposure /*= 1.0f*/) const
{
	const sgl::Error& error = sgl::Error::GetInstance();
	sgl::Frame frame{};
	sgl::Render render{};
	auto size = display->GetSize();
	frame.BindAttach(render);
	render.BindStorage(size);

	auto texture_out = std::make_shared<sgl::Texture>(
		size,
		sgl::PixelElementSize::HALF,
		sgl::PixelStructure::RGB);

	// Set the view port for rendering.
	glViewport(0, 0, size.first, size.second);
	error.Display(__FILE__, __LINE__ - 1);

	// Clear the screen.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	error.Display(__FILE__, __LINE__ - 1);

	frame.BindTexture(*texture_out);

	sgl::TextureManager texture_manager;
	texture_manager.AddTexture("Display", display);
	texture_manager.AddTexture("GaussianBlur", gaussian_blur);
	auto program = sgl::CreateProgram("Combine");
	program->UniformFloat("exposure", exposure);
	auto quad = sgl::CreateQuadMesh(program);
	quad->SetTextures({ "Display", "GaussianBlur" });
	quad->Draw(texture_manager);

	return texture_out;
}
