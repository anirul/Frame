#include "Fill.h"
#include <assert.h>

namespace sgl {

	namespace {
		// Get the 6 view for the cube map.
		const std::array<glm::mat4, 6> views_cubemap =
		{
			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(1.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, -1.0f, 0.0f)),
			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(-1.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, -1.0f, 0.0f)),
			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 1.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, 1.0f)),
			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, -1.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, -1.0f)),
			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, 1.0f),
				glm::vec3(0.0f, -1.0f, 0.0f)),
			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, -1.0f),
				glm::vec3(0.0f, -1.0f, 0.0f))
		};
	}

	void FillProgramMultiTexture(
		std::vector<std::shared_ptr<Texture>>& out_textures,
		const std::map<std::string, std::shared_ptr<Texture>>& in_textures,
		const std::shared_ptr<ProgramInterface> program)
	{
		FillProgramMultiTextureMipmap(
			out_textures,
			in_textures,
			program,
			0,
			[](const int, const std::shared_ptr<ProgramInterface>) {});
	}

	void FillProgramMultiTextureMipmap(
		std::vector<std::shared_ptr<Texture>>& out_textures,
		const std::map<std::string, std::shared_ptr<Texture>>& in_textures,
		const std::shared_ptr<ProgramInterface> program,
		const int mipmap,
		const std::function<void(
			const int mipmap,
			const std::shared_ptr<ProgramInterface> program)> func /*=
		[](const int, const std::shared_ptr<sgl::ProgramInterface>) {}*/)
	{
		auto& error = Error::GetInstance();
		assert(out_textures.size());
		auto size = out_textures[0]->GetSize();
		FrameBuffer frame{};
		RenderBuffer render{};
		ScopedBind scoped_frame(frame);
		ScopedBind scoped_render(render);
		render.CreateStorage(size);
		frame.AttachRender(render);
		frame.DrawBuffers(static_cast<std::uint32_t>(out_textures.size()));
		int max_mipmap = (mipmap <= 0) ? 1 : mipmap;
		if (max_mipmap > 1) {
			for (const auto& texture : out_textures)
			{
				texture->BindEnableMipmap();
			}
		}
		glm::mat4 projection = glm::perspective(
			glm::radians(90.0f),
			1.0f,
			0.1f,
			10.0f);
		auto quad = CreateQuadStaticMesh();
		auto material = std::make_shared<Material>();
		for (const auto& p : in_textures)
		{
			material->AddTexture(p.first, p.second);
		}
		quad->SetMaterial(material);
		std::pair<uint32_t, uint32_t> temporary_size = size;
		for (int mipmap_level = 0; mipmap_level < max_mipmap; ++mipmap_level)
		{
			func(mipmap_level, program);
			double fact = std::pow(0.5, mipmap_level);
			temporary_size.first =
				static_cast<std::uint32_t>(size.first * fact);
			temporary_size.second =
				static_cast<std::uint32_t>(size.second * fact);
			glViewport(0, 0, temporary_size.first, temporary_size.second);
			error.Display(__FILE__, __LINE__ - 1);
			for (int i = 0; i < out_textures.size(); ++i)
			{
				frame.AttachTexture(
					*out_textures[i],
					FrameBuffer::GetFrameColorAttachment(i),
					mipmap_level);
			}
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			error.Display(__FILE__, __LINE__ - 1);

			program->Use();
			program->Uniform("projection", projection);
			program->Uniform("view", glm::mat4(1.0f));
			program->Uniform("model", glm::mat4(1.0f));

			quad->Draw(program);
		}
	}

	void FillProgramMultiTextureCubeMap(
		std::vector<std::shared_ptr<Texture>>& out_textures,
		const std::map<std::string, std::shared_ptr<Texture>>& in_textures,
		const std::shared_ptr<ProgramInterface> program)
	{
		FillProgramMultiTextureCubeMapMipmap(
			out_textures,
			in_textures,
			program,
			0,
			[](const int, const std::shared_ptr<ProgramInterface>) {});
	}

	void FillProgramMultiTextureCubeMapMipmap(
		std::vector<std::shared_ptr<Texture>>& out_textures,
		const std::map<std::string, std::shared_ptr<Texture>>& in_textures,
		const std::shared_ptr<ProgramInterface> program,
		const int mipmap,
		const std::function<void(
			const int mipmap,
			const std::shared_ptr<sgl::ProgramInterface> program)> func /*=
		[](const int, const std::shared_ptr<sgl::ProgramInterface>) {}*/)
	{
		auto& error = Error::GetInstance();
		assert(out_textures.size());
		auto size = out_textures[0]->GetSize();
		FrameBuffer frame{};
		RenderBuffer render{};
		ScopedBind scoped_frame(frame);
		ScopedBind scoped_render(render);
		frame.AttachRender(render);
		frame.DrawBuffers(static_cast<std::uint32_t>(out_textures.size()));
		int max_mipmap = (mipmap <= 0) ? 1 : mipmap;
		if (max_mipmap > 1)
		{
			for (const auto& texture : out_textures)
			{
				texture->BindEnableMipmap();
			}
		}
		glm::mat4 projection = glm::perspective(
			glm::radians(90.0f),
			1.0f,
			0.1f,
			10.0f);
		auto cube = CreateCubeStaticMesh();
		auto material = std::make_shared<Material>();
		for (const auto& p : in_textures)
		{
			material->AddTexture(p.first, p.second);
		}
		cube->SetMaterial(material);
		std::pair<std::uint32_t, std::uint32_t> temporary_size = { 0, 0 };
		for (int mipmap_level = 0; mipmap_level < max_mipmap; ++mipmap_level)
		{
			func(mipmap_level, program);
			double fact = std::pow(0.5, mipmap_level);
			temporary_size.first =
				static_cast<std::uint32_t>(size.first * fact);
			temporary_size.second =
				static_cast<std::uint32_t>(size.second * fact);
			render.CreateStorage(temporary_size);
			frame.AttachRender(render);
			glViewport(0, 0, temporary_size.first, temporary_size.second);
			error.Display(__FILE__, __LINE__ - 1);
			int cubemap_element = 0;
			for (const auto& view : views_cubemap)
			{
				for (int i = 0; i < out_textures.size(); ++i)
				{
					frame.AttachTexture(
						*out_textures[i],
						FrameBuffer::GetFrameColorAttachment(i),
						mipmap_level,
						FrameBuffer::GetFrameTextureType(cubemap_element));
				}
				cubemap_element++;
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				error.Display(__FILE__, __LINE__ - 1);

				program->Use();
				program->Uniform("projection", projection);
				program->Uniform("view", view);
				program->Uniform("model", glm::mat4(1.0f));

				cube->Draw(program);
			}
		}
	}

} // End namespace sgl.
