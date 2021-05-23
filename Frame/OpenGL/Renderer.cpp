#include "Renderer.h"
#include <stdexcept>
#include <fmt/core.h>
#include <GL/glew.h>
#include "Frame/NodeStaticMesh.h"
#include "Frame/OpenGL/Material.h"
#include "Frame/OpenGL/File/LoadProgram.h"

namespace frame::opengl {

	Renderer::Renderer(
		LevelInterface* level, 
		UniformInterface* uniform_interface,
		std::pair<std::uint32_t, std::uint32_t> size) :
		level_(level),
		uniform_interface_(uniform_interface)
	{
		if (!size.first || !size.second) 
		{
			throw std::runtime_error(
				fmt::format("No size ({}, {})?", size.first, size.second));
		}
		render_buffer_.CreateStorage(size);
		frame_buffer_.AttachRender(render_buffer_);
		auto program = file::LoadProgram("Display");
		if (!program)
			throw std::runtime_error("No program!");
		program->SetDepthTest(false);
		auto material = std::make_shared<Material>();
		display_program_id_ = level_->AddProgram("DisplayProgram", program);
		if (!level_) 
			throw std::runtime_error("No level!");
		display_material_id_ = level_->AddMaterial("DisplayMaterial", material);
		auto out_texture_id = level->GetDefaultOutputTextureId();
		auto out_texture = level_->GetTextureMap().at(out_texture_id);
		material->SetProgramId(display_program_id_);
		material->AddTextureId(out_texture_id, "Display");
	}

	void Renderer::RenderNode(EntityId node_id, const double dt/* = 0.0*/)
	{
		// Check current node.
		auto node = level_->GetSceneNodeMap().at(node_id);
		// Try to case to a node static mesh.
		auto node_static_mesh = std::dynamic_pointer_cast<NodeStaticMesh>(node);
		if (!node_static_mesh) return;
		// Store the material id in case the mesh doesn't have one.
		auto node_material_id = node_static_mesh->GetMaterialId();
		auto mesh_id = node->GetLocalMesh();
		if (!mesh_id) return;
		auto static_mesh = level_->GetStaticMeshMap().at(mesh_id);
		// Get the mesh material id.
		auto material_id = static_mesh->GetMaterialId();
		// If no material is put the node material id in the mesh.
		if (!material_id) static_mesh->SetMaterialId(node_material_id);
		RenderMesh(static_mesh.get(), node->GetLocalModel(dt), dt);
	}

	void Renderer::RenderChildren(EntityId node_id, const double dt/* = 0.0*/)
	{
		RenderNode(node_id, dt);
		// Loop into the child of the root node.
		const std::vector<EntityId> list = level_->GetChildList(node_id);
		for (const auto& id : list)	RenderChildren(id, dt);
	}

	void Renderer::RenderFromRootNode(const double dt/* = 0.0*/)
	{
		const EntityId root_id = level_->GetDefaultRootSceneNodeId();
		RenderChildren(root_id, dt);
	}

	void Renderer::RenderMesh(
		StaticMeshInterface* static_mesh,
		const glm::mat4& model_mat/* = glm::mat4(1.0f)*/,
		const double dt/* = 0.0*/)
	{
		if (!static_mesh)
			throw std::runtime_error("StaticMesh ptr doesn't exist.");
		auto material_id = static_mesh->GetMaterialId();
		auto material = level_->GetMaterialMap().at(material_id);
		if (!material)
			throw std::runtime_error("No material!");
		auto program_id = material->GetProgramId();
		auto program = level_->GetProgramMap().at(program_id);
		if (!program)
			throw std::runtime_error("Program ptr doesn't exist.");
		assert(program->GetOutputTextureIds().size());
		SetDepthTest(program->GetDepthTest());
		program->Use(uniform_interface_);
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
		program->Uniform("model", model_mat);

		for (const auto& id : material->GetIds())
		{
			const auto p = material->EnableTextureId(id);
			level_->GetTextureMap().at(id)->Bind(p.second);
			program->Uniform(p.first, p.second);
		}
		
		glBindVertexArray(static_mesh->GetId());
		error_.Display(__FILE__, __LINE__ - 1);

		auto index_buffer = level_->GetBufferMap().at(
			static_mesh->GetIndexBufferId());
		index_buffer->Bind();
		glDrawElements(
			GL_TRIANGLES,
			static_cast<GLsizei>(static_mesh->GetIndexSize()) / 
				sizeof(std::int32_t),
			GL_UNSIGNED_INT,
			nullptr);
		error_.Display(__FILE__, __LINE__ - 5);
		index_buffer->UnBind();

		program->UnUse();
		glBindVertexArray(0);
		error_.Display(__FILE__, __LINE__ - 1);

		for (const auto id : material->GetIds())
		{
			level_->GetTextureMap().at(id)->UnBind();
		}
		material->DisableAll();

		if (static_mesh->IsClearBuffer())
		{
			glClear(GL_DEPTH_BUFFER_BIT);
			error_.Display(__FILE__, __LINE__ - 1);
		}
	}

	void Renderer::Display()
	{
		auto quad = level_->GetStaticMeshMap().at(
			level_->GetDefaultStaticMeshQuadId());
		quad->SetMaterialId(display_material_id_);
		auto program = level_->GetProgramMap().at(display_program_id_);
		SetDepthTest(program->GetDepthTest());
		program->Use(uniform_interface_);
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
			static_cast<GLsizei>(quad->GetIndexSize()) / sizeof(std::int32_t),
			GL_UNSIGNED_INT,
			nullptr);
		error_.Display(__FILE__, __LINE__ - 5);
		index_buffer->UnBind();

		program->UnUse();
		glBindVertexArray(0);
		error_.Display(__FILE__, __LINE__ - 1);

		for (const auto id : material->GetIds())
		{
			level_->GetTextureMap().at(id)->UnBind();
		}
		material->DisableAll();
		quad->SetMaterialId(0);
	}

	void Renderer::SetDepthTest(bool enable) const
	{
		if (enable)
		{
			glEnable(GL_DEPTH_TEST);
			error_.Display(__FILE__, __LINE__ - 1);
		}
		else
		{
			glDisable(GL_DEPTH_TEST);
			error_.Display(__FILE__, __LINE__ - 1);
		}
	}

} // End namespace frame::opengl.
