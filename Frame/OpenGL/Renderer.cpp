#include "Renderer.h"

#include <stdexcept>
#include <fmt/core.h>
#include <GL/glew.h>

#include "Frame/NodeStaticMesh.h"
#include "Frame/UniformWrapper.h"
#include "Frame/OpenGL/Material.h"
#include "Frame/OpenGL/File/LoadProgram.h"

namespace frame::opengl {

	Renderer::Renderer(
		LevelInterface* level,
		std::pair<std::uint32_t, std::uint32_t> size) :
		level_(level)
	{
		if (!size.first || !size.second) 
		{
			throw std::runtime_error(
				fmt::format("No size ({}, {})?", size.first, size.second));
		}
		render_buffer_.CreateStorage(size);
		frame_buffer_.AttachRender(render_buffer_);
		auto maybe_program = file::LoadProgram("Display");
		if (!maybe_program)	throw std::runtime_error("No program!");
		auto program = std::move(maybe_program.value());
		program->SetDepthTest(false);
		auto material = std::make_unique<Material>();
		program->SetName("DisplayProgram");
		if (!level_) throw std::runtime_error("No level!");
		auto maybe_display_program_id = level_->AddProgram(std::move(program));
		if (!maybe_display_program_id) 
			throw std::runtime_error("No display program id.");
		display_program_id_ = maybe_display_program_id.value();
		material->SetName("DisplayMaterial");
		auto maybe_display_material_id = 
			level_->AddMaterial(std::move(material));
		if (!maybe_display_material_id) 
			throw std::runtime_error("No display material id.");
		display_material_id_ = maybe_display_material_id.value();
		auto maybe_out_texture_id = level->GetDefaultOutputTextureId();
		if (!maybe_out_texture_id) 
			throw std::runtime_error("No output texture id.");
		auto out_texture_id = maybe_out_texture_id.value();
		auto out_texture = level_->GetTextureFromId(out_texture_id);
		// Get material from level as material was moved away.
		level_->GetMaterialFromId(
			display_material_id_)->SetProgramId(display_program_id_);
		if (!level_->GetMaterialFromId(
			display_material_id_)->AddTextureId(out_texture_id, "Display"))
		{
			throw std::runtime_error("Couldn't add texture to material.");
		}
	}

	void Renderer::RenderNode(EntityId node_id, double dt/* = 0.0*/)
	{
		// Check current node.
		auto node = level_->GetSceneNodeFromId(node_id);
		// Try to cast to a node static mesh.
		auto node_static_mesh = dynamic_cast<NodeStaticMesh*>(node);
		if (!node_static_mesh) return;
		// Store the material id in case the mesh doesn't have one.
		auto node_material_id = node_static_mesh->GetMaterialId();
		auto mesh_id = node->GetLocalMesh();
		if (!mesh_id) return;
		auto static_mesh = level_->GetStaticMeshFromId(mesh_id);
		// Get the mesh material id.
		auto material_id = static_mesh->GetMaterialId();
		// If no material is put the node material id in the mesh.
		if (!material_id) static_mesh->SetMaterialId(node_material_id);
		RenderMesh(static_mesh, node->GetLocalModel(dt), dt);
	}

	void Renderer::RenderChildren(EntityId node_id, double dt/* = 0.0*/)
	{
		RenderNode(node_id, dt);
		// Loop into the child of the root node.
		auto maybe_list = level_->GetChildList(node_id);
		if (!maybe_list) throw std::runtime_error("No child list.");
		auto list = maybe_list.value();
		for (const auto& id : list)	RenderChildren(id, dt);
	}

	void Renderer::RenderFromRootNode(double dt/* = 0.0*/)
	{
		auto maybe_root_id = level_->GetDefaultRootSceneNodeId();
		if (!maybe_root_id) throw std::runtime_error("No root id.");
		EntityId root_id = maybe_root_id.value();
		RenderChildren(root_id, dt);
	}

	void Renderer::RenderMesh(
		StaticMeshInterface* static_mesh,
		glm::mat4 model_mat/* = glm::mat4(1.0f)*/,
		double dt/* = 0.0*/)
	{
		if (!static_mesh)
			throw std::runtime_error("StaticMesh ptr doesn't exist.");
		auto material_id = static_mesh->GetMaterialId();
		auto material = level_->GetMaterialFromId(material_id);
		if (!material) throw std::runtime_error("No material!");
		auto program_id = material->GetProgramId();
		auto program = level_->GetProgramFromId(program_id);
		if (!program) throw std::runtime_error("Program ptr doesn't exist.");
		assert(program->GetOutputTextureIds().size());
		SetDepthTest(program->GetDepthTest());
		UniformWrapper uniform_wrapper(level_->GetDefaultCamera());
		uniform_wrapper.SetTime(dt);
		program->Use(&uniform_wrapper);
		auto texture_out_ids = program->GetOutputTextureIds();
		auto texture_ref = 
			level_->GetTextureFromId(*texture_out_ids.cbegin());
		auto size = texture_ref->GetSize();

		glViewport(0, 0, size.first, size.second);
		error_.Display(__FILE__, __LINE__ - 1);

		ScopedBind scoped_frame(frame_buffer_);
		int i = 0;
		for (const auto& texture_id : program->GetOutputTextureIds())
		{
			// TODO(anirul): Check the mipmap level (last parameter)!
			frame_buffer_.AttachTexture(
				level_->GetTextureFromId(texture_id)->GetId(),
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
			level_->GetTextureFromId(id)->Bind(p.second);
			program->Uniform(p.first, p.second);
		}
		
		glBindVertexArray(static_mesh->GetId());
		error_.Display(__FILE__, __LINE__ - 1);

		auto index_buffer = level_->GetBufferFromId(
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
			level_->GetTextureFromId(id)->UnBind();
		}
		material->DisableAll();

		if (static_mesh->IsClearBuffer())
		{
			glClear(GL_DEPTH_BUFFER_BIT);
			error_.Display(__FILE__, __LINE__ - 1);
		}
	}

	void Renderer::Display(double dt/* = 0.0*/)
	{
		auto maybe_quad_id = level_->GetDefaultStaticMeshQuadId();
		if (!maybe_quad_id) throw std::runtime_error("No quad id.");
		auto quad = level_->GetStaticMeshFromId(maybe_quad_id.value());
		quad->SetMaterialId(display_material_id_);
		auto program = level_->GetProgramFromId(display_program_id_);
		SetDepthTest(program->GetDepthTest());
		UniformWrapper uniform_wrapper(level_->GetDefaultCamera());
		uniform_wrapper.SetTime(dt);
		program->Use(&uniform_wrapper);
		auto material = level_->GetMaterialFromId(display_material_id_);

		for (const auto id : material->GetIds())
		{
			const auto p = material->EnableTextureId(id);
			level_->GetTextureFromId(id)->Bind(p.second);
			program->Uniform(p.first, p.second);
		}

		glBindVertexArray(quad->GetId());
		error_.Display(__FILE__, __LINE__ - 1);

		auto index_buffer = level_->GetBufferFromId(quad->GetIndexBufferId());
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
			level_->GetTextureFromId(id)->UnBind();
		}
		material->DisableAll();
		quad->SetMaterialId(0);
	}

	void Renderer::SetDepthTest(bool enable)
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
