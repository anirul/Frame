#include "Rendering.h"
#include <stdexcept>
#include <fmt/core.h>
#include <GL/glew.h>
#include "Frame/OpenGL/FrameBuffer.h"
#include "Frame/OpenGL/RenderBuffer.h"

namespace frame::opengl {

	void Rendering::DisplayMesh(
		LevelInterface* level,
		ProgramInterface* program, 
		StaticMeshInterface* static_mesh, 
		const double dt /*= 0.0*/)
	{
		if (!level)
			throw std::runtime_error("Level ptr doesn't exist.");
		if (!program)
			throw std::runtime_error("Program ptr doesn't exist.");
		if (!static_mesh)
			throw std::runtime_error("StatiMesh ptr doesn't exist.");

		assert(program->GetOutputTextureIds().size());
		auto texture_out_ids = program->GetOutputTextureIds();
		auto texture_ref = level->GetTextureMap().at(*texture_out_ids.cbegin());
		auto size = texture_ref->GetSize();
		FrameBuffer frame_buffer{};
		RenderBuffer render_buffer{};
		ScopedBind scoped_frame(frame_buffer);
		ScopedBind scoped_render(render_buffer);
		render_buffer.CreateStorage(size);
		frame_buffer.AttachRender(render_buffer);
		frame_buffer.DrawBuffers(
			static_cast<std::uint32_t>(texture_out_ids.size()));
		int i = 0;
		for (const auto& texture_id : program->GetOutputTextureIds())
		{
			// TODO(anirul): Check the mipmap level (last parameter)!
			frame_buffer.AttachTexture(
				level->GetTextureMap().at(texture_id)->GetId(),
				FrameBuffer::GetFrameColorAttachment(i),
				0);
			i++;
		}

		program->Use();
		program->Uniform("projection", projection_);
		program->Uniform("view", view_);
		program->Uniform("model", model_);

		auto material_id = static_mesh->GetMaterialId();
		if (material_id)
		{
			auto material = level->GetMaterialMap().at(material_id);
			for (const auto& id : material->GetIds())
			{
				const auto p = material->EnableTextureId(id);
				program->Uniform(p.first, p.second);
			}
		}

		glBindVertexArray(static_mesh->GetId());
		error_.Display(__FILE__, __LINE__ - 1);

		auto index_buffer = level->GetBufferMap().at(
			static_mesh->GetIndexBufferId());
		index_buffer->Bind();
		glDrawElements(
			GL_TRIANGLES,
			static_cast<GLsizei>(static_mesh->GetIndexSize()),
			GL_UNSIGNED_INT,
			nullptr);
		error_.Display(__FILE__, __LINE__ - 5);
		index_buffer->UnBind();

		glBindVertexArray(0);
		error_.Display(__FILE__, __LINE__ - 1);

		if (material_id)
		{
			auto material = level->GetMaterialMap().at(material_id);
			material->DisableAll();
		}

		if (static_mesh->IsClearBuffer())
		{
			glClear(GL_DEPTH_BUFFER_BIT);
			error_.Display(__FILE__, __LINE__ - 1);
		}
	}

} // End namespace frame::opengl.
