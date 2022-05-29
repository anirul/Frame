#include "Fill.h"

#include <array>
#include <assert.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Frame/OpenGL/FrameBuffer.h"
#include "Frame/OpenGL/RenderBuffer.h"
#include "Frame/OpenGL/Renderer.h"
#include "Frame/OpenGL/StaticMesh.h"

namespace frame::opengl {

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
		const std::shared_ptr<LevelInterface> level,
		const std::shared_ptr<ProgramInterface> program)
	{
		FillProgramMultiTextureMipmap(
			level,
			program,
			0,
			[](const int, const std::shared_ptr<ProgramInterface>) {});
	}

	void FillProgramMultiTextureMipmap(
		const std::shared_ptr<LevelInterface> level,
		const std::shared_ptr<ProgramInterface> program,
		const int mipmap,
		const std::function<void(
			const int mipmap,
			const std::shared_ptr<ProgramInterface> program)> func /*=
		[](const int, const std::shared_ptr<sgl::ProgramInterface>) {}*/)
	{
		assert(program->GetOutputTextureIds().size());
		auto texture_out_ids = program->GetOutputTextureIds();
		auto texture_ref = level->GetTextureFromId(*texture_out_ids.cbegin());
		auto size = texture_ref->GetSize();
		FrameBuffer frame{};
		RenderBuffer render{};
		ScopedBind scoped_frame(frame);
		ScopedBind scoped_render(render);
		render.CreateStorage(size);
		frame.AttachRender(render);
		frame.DrawBuffers(static_cast<std::uint32_t>(texture_out_ids.size()));
		int max_mipmap = (mipmap <= 0) ? 1 : mipmap;
		if (max_mipmap > 1) 
		{
			for (const auto& texture_id : texture_out_ids)
			{
				auto texture = level->GetTextureFromId(texture_id);
				texture->Bind();
				texture->EnableMipmap();
			}
		}
		glm::mat4 projection = glm::perspective(
			glm::radians(90.0f),
			1.0f,
			0.1f,
			10.0f);
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
			int i = 0; 
			for (const auto& texture_id : program->GetOutputTextureIds())
			{
				frame.AttachTexture(
					level->GetTextureFromId(texture_id)->GetId(),
					FrameBuffer::GetFrameColorAttachment(i),
					FrameTextureType::TEXTURE_2D,
					mipmap_level);
					i++;
			}
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			Renderer renderer(
				level.get(), 
				temporary_size);
			renderer.SetProjection(projection);
			auto maybe_id = level->GetDefaultStaticMeshQuadId();
			if (!maybe_id) 
			{
				throw std::runtime_error(
					"Invalid default static mesh quad id.");
			}
			renderer.RenderMesh(level->GetStaticMeshFromId(maybe_id.value()));
		}
	}

	void FillProgramMultiTextureCubeMap(
		const std::shared_ptr<LevelInterface> level,
		const std::shared_ptr<ProgramInterface> program)
	{
		FillProgramMultiTextureCubeMapMipmap(
			level,
			program,
			0,
			[](const int, const std::shared_ptr<ProgramInterface>) {});
	}

	void FillProgramMultiTextureCubeMapMipmap(
		const std::shared_ptr<LevelInterface> level,
		const std::shared_ptr<ProgramInterface> program,
		const int mipmap,
		const std::function<void(
			const int mipmap,
			const std::shared_ptr<ProgramInterface> program)> func /*=
		[](const int, const std::shared_ptr<sgl::ProgramInterface>) {}*/)
	{
		assert(program->GetOutputTextureIds().size());
		auto texture_out_ids = program->GetOutputTextureIds();
		auto texture_ref = level->GetTextureFromId(*texture_out_ids.cbegin());
		auto size = texture_ref->GetSize();
		FrameBuffer frame{};
		RenderBuffer render{};
		ScopedBind scoped_frame(frame);
		ScopedBind scoped_render(render);
		frame.AttachRender(render);
		frame.DrawBuffers(static_cast<std::uint32_t>(texture_out_ids.size()));
		int max_mipmap = (mipmap <= 0) ? 1 : mipmap;
		if (max_mipmap > 1)
		{
			for (const auto& texture_id : texture_out_ids)
			{
				auto texture = level->GetTextureFromId(texture_id);
				texture->Bind();
				texture->EnableMipmap();
			}
		}
		glm::mat4 projection = glm::perspective(
			glm::radians(90.0f),
			1.0f,
			0.1f,
			10.0f);
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
			int cubemap_element = 0;
			for (const auto& view : views_cubemap)
			{
				int i = 0;
				for (const auto& texture_id : program->GetOutputTextureIds())
				{
					frame.AttachTexture(
						level->GetTextureFromId(texture_id)->GetId(),
						FrameBuffer::GetFrameColorAttachment(i),
						FrameTextureType::TEXTURE_2D,
						mipmap_level);
				}
				cubemap_element++;
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				Renderer renderer(
					level.get(), 
					temporary_size);
				renderer.SetProjection(projection);
				renderer.SetView(view);
				auto maybe_id = level->GetDefaultStaticMeshCubeId();
				if (!maybe_id)
				{
					throw std::runtime_error(
						"Invalid default static mesh cube id.");
				}
				renderer.RenderMesh(
					level->GetStaticMeshFromId(maybe_id.value()));
			}
		}
	}

} // End namespace frame::opengl.
