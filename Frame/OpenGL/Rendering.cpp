#include "Rendering.h"
#include <stdexcept>
#include <fmt/core.h>
#include <GL/glew.h>
#include "Frame/OpenGL/Material.h"
#include "Frame/OpenGL/File/LoadProgram.h"

namespace frame::opengl {

	Rendering::Rendering(
		std::shared_ptr<LevelInterface> level, 
		std::pair<std::uint32_t, std::uint32_t> size) :
		level_(level)
	{
		assert(size.first);
		assert(size.second);
		render_buffer_.CreateStorage(size);
		frame_buffer_.AttachRender(render_buffer_);
		auto program = file::LoadProgram("Display");
		auto material = std::make_shared<Material>();
		display_program_id_ = level_->AddProgram("DisplayProgram", program);
		display_material_id_ = level_->AddMaterial("DisplayMaterial", material);
		auto out_texture = level_->GetTextureMap().at(
			level_->GetDefaultOutputTextureId());
		material->AddTextureId(level_->GetDefaultOutputTextureId(),	"Display");
	}

	void Rendering::RenderMesh(
		const UniformInterface* uniform_interface,
		ProgramInterface* program, 
		StaticMeshInterface* static_mesh, 
		const double dt /*= 0.0*/)
	{
		if (!program)
			throw std::runtime_error("Program ptr doesn't exist.");
		if (!static_mesh)
			throw std::runtime_error("StaticMesh ptr doesn't exist.");

		assert(program->GetOutputTextureIds().size());
		program->Use(uniform_interface);
		auto texture_out_ids = program->GetOutputTextureIds();
		auto texture_ref = 
			level_->GetTextureMap().at(*texture_out_ids.cbegin());
		auto size = texture_ref->GetSize();

		glViewport(0, 0, size.first, size.second);
		error_.Display(__FILE__, __LINE__ - 1);

		ScopedBind scoped_frame(frame_buffer_);
		int i = 0;
		for (const auto& texture_id : program->GetOutputTextureIds())
		{
			// TODO(anirul): Check the mipmap level (last parameter)!
			frame_buffer_.AttachTexture(
				level_->GetTextureMap().at(texture_id)->GetId(),
				FrameBuffer::GetFrameColorAttachment(i),
				0);
			i++;
		}
		frame_buffer_.DrawBuffers(
			static_cast<std::uint32_t>(texture_out_ids.size()));

		program->Uniform("projection", projection_);
		program->Uniform("view", view_);
		program->Uniform("model", model_);

		auto material_id = static_mesh->GetMaterialId();
		if (material_id)
		{
			auto material = level_->GetMaterialMap().at(material_id);
			for (const auto& id : material->GetIds())
			{
				const auto p = material->EnableTextureId(id);
				level_->GetTextureMap().at(id)->Bind(p.second);
				program->Uniform(p.first, p.second);
			}
		}

		glBindVertexArray(static_mesh->GetId());
		error_.Display(__FILE__, __LINE__ - 1);

		auto index_buffer = level_->GetBufferMap().at(
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
			auto material = level_->GetMaterialMap().at(material_id);
			for (const auto id : material->GetIds())
			{
				level_->GetTextureMap().at(id)->UnBind();
			}
			material->DisableAll();
		}

		if (static_mesh->IsClearBuffer())
		{
			glClear(GL_DEPTH_BUFFER_BIT);
			error_.Display(__FILE__, __LINE__ - 1);
		}
	}

	void Rendering::Display(const UniformInterface* uniform_interface)
	{
		auto quad = level_->GetStaticMeshMap().at(
			level_->GetDefaultStaticMeshQuadId());
		quad->SetMaterialId(display_material_id_);
		auto program = level_->GetProgramMap().at(display_program_id_);
		program->Use(uniform_interface);
		auto material = level_->GetMaterialMap().at(display_material_id_);

		for (const auto id : material->GetIds())
		{
			const auto p = material->EnableTextureId(id);
			level_->GetTextureMap().at(id)->Bind(p.second);
			program->Uniform(p.first, p.second);
		}

		glBindVertexArray(quad->GetId());
		error_.Display(__FILE__, __LINE__ - 1);

		auto index_buffer = level_->GetBufferMap().at(quad->GetIndexBufferId());
		index_buffer->Bind();
		glDrawElements(
			GL_TRIANGLES,
			static_cast<GLsizei>(quad->GetIndexSize()),
			GL_UNSIGNED_INT,
			nullptr);
		error_.Display(__FILE__, __LINE__ - 5);
		index_buffer->UnBind();

		glBindVertexArray(0);
		error_.Display(__FILE__, __LINE__ - 1);

		for (const auto id : material->GetIds())
		{
			level_->GetTextureMap().at(id)->UnBind();
		}
		material->DisableAll();
		quad->SetMaterialId(0);
	}

} // End namespace frame::opengl.
