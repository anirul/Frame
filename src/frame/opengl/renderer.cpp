#include "renderer.h"

#include <GL/glew.h>
#include <fmt/core.h>

#include <stdexcept>

#include "frame/node_matrix.h"
#include "frame/node_static_mesh.h"
#include "frame/opengl/file/load_program.h"
#include "frame/opengl/material.h"
#include "frame/opengl/static_mesh.h"
#include "frame/opengl/texture.h"
#include "frame/opengl/texture_cube_map.h"
#include "frame/uniform_wrapper.h"

namespace frame::opengl {

Renderer::Renderer(LevelInterface* level, std::pair<std::uint32_t, std::uint32_t> size)
    : level_(level) {
    if (!size.first || !size.second) {
        throw std::runtime_error(fmt::format("No size ({}, {})?", size.first, size.second));
    }
    render_buffer_.CreateStorage(size);
    frame_buffer_.AttachRender(render_buffer_);
    auto program = file::LoadProgram("display");
    if (!program) throw std::runtime_error("No program!");
    auto material = std::make_unique<Material>();
    program->SetName("DisplayProgram");
    if (!level_) throw std::runtime_error("No level!");
    auto maybe_display_program_id = level_->AddProgram(std::move(program));
    if (!maybe_display_program_id) throw std::runtime_error("No display program id.");
    display_program_id_ = maybe_display_program_id;
    material->SetName("DisplayMaterial");
    auto maybe_display_material_id = level_->AddMaterial(std::move(material));
    if (!maybe_display_material_id) throw std::runtime_error("No display material id.");
    display_material_id_      = maybe_display_material_id;
    auto maybe_out_texture_id = level->GetDefaultOutputTextureId();
    if (!maybe_out_texture_id) throw std::runtime_error("No output texture id.");
    auto out_texture_id = maybe_out_texture_id;
    auto out_texture    = level_->GetTextureFromId(out_texture_id);
    // Get material from level as material was moved away.
    level_->GetMaterialFromId(display_material_id_)->SetProgramId(display_program_id_);
    if (!level_->GetMaterialFromId(display_material_id_)->AddTextureId(out_texture_id, "Display")) {
        throw std::runtime_error("Couldn't add texture to material.");
    }
}

void Renderer::RenderNode(EntityId node_id, EntityId material_id /* = NullId*/,
                          double dt /* = 0.0*/) {
    // Check current node.
    auto node = level_->GetSceneNodeFromId(node_id);
    // Try to cast to a node static mesh.
    auto node_static_mesh = dynamic_cast<NodeStaticMesh*>(node);
    if (!node_static_mesh) return;
    auto mesh_id = node->GetLocalMesh();
    // In case no mesh then this is a clear event.
    if (!mesh_id) {
        GLbitfield bit_field       = 0;
        std::uint32_t clean_buffer = node_static_mesh->GetCleanBuffer();
        if (clean_buffer | proto::CleanBuffer::CLEAR_COLOR) bit_field += GL_COLOR_BUFFER_BIT;
        if (clean_buffer | proto::CleanBuffer::CLEAR_DEPTH) bit_field += GL_DEPTH_BUFFER_BIT;
        if (bit_field) glClear(bit_field);
        return;
    }
    auto static_mesh            = level_->GetStaticMeshFromId(mesh_id);
    MaterialInterface* material = nullptr;
    // Try to find the material for the mesh.
    if (material_id) {
        material = level_->GetMaterialFromId(material_id);
    }
    RenderMesh(static_mesh, material, node->GetLocalModel(dt), dt);
}

void Renderer::RenderChildren(EntityId node_id, double dt /* = 0.0*/) {
    RenderNode(node_id, NullId, dt);
    // Loop into the child of the root node.
    auto maybe_list = level_->GetChildList(node_id);
    if (!maybe_list) throw std::runtime_error("No child list.");
    auto list = maybe_list.value();
    for (const auto& id : list) {
        RenderChildren(id, dt);
    }
}

void Renderer::RenderFromRootNode(double dt /* = 0.0*/) {
    auto maybe_root_id = level_->GetDefaultRootSceneNodeId();
    if (!maybe_root_id) return;
    EntityId root_id = maybe_root_id;
    RenderChildren(root_id, dt);
}

void Renderer::RenderMesh(StaticMeshInterface* static_mesh,
                          MaterialInterface* material /* = nullptr*/,
                          glm::mat4 model_mat /* = glm::mat4(1.0f)*/, double dt /* = 0.0*/) {
    if (!static_mesh) throw std::runtime_error("StaticMesh ptr doesn't exist.");
    if (!material) throw std::runtime_error("No material!");
    auto program_id = material->GetProgramId();
    auto program    = level_->GetProgramFromId(program_id);
    if (!program) throw std::runtime_error("Program ptr doesn't exist.");
    last_program_id_ = program_id;
    assert(program->GetOutputTextureIds().size());

    // In case the camera doesn't exist it will create a basic one.
    UniformWrapper uniform_wrapper(level_->GetDefaultCamera());
    uniform_wrapper.SetModel(model_mat);
    uniform_wrapper.SetTime(dt);
    // Get the stream from level to the uniform float wrapper.
    for (const EntityId id : level_->GetUniformFloatStreamIds()) {
        auto* stream     = level_->GetUniformFloatStreamFromId(id);
        auto string_name = stream->GetName();
        uniform_wrapper.SetValueFloatFromStream(string_name, stream->ExtractVector(),
                                                stream->GetSize());
    }
    // Get the stream from level to the uniform int wrapper.
    for (const EntityId id : level_->GetUniformIntStreamIds()) {
        auto* stream     = level_->GetUniformIntStreamFromId(id);
        auto string_name = stream->GetName();
        uniform_wrapper.SetValueIntFromStream(string_name, stream->ExtractVector(),
                                              stream->GetSize());
    }
    program->Use(&uniform_wrapper);

    auto texture_out_ids = program->GetOutputTextureIds();
    auto texture_ref     = level_->GetTextureFromId(*texture_out_ids.cbegin());
    auto size            = texture_ref->GetSize();

    glViewport(0, 0, size.first, size.second);

    ScopedBind scoped_frame(frame_buffer_);
    int i = 0;
    for (const auto& texture_id : program->GetOutputTextureIds()) {
        if (level_->GetTextureFromId(texture_id)->IsCubeMap()) {
            auto* opengl_texture =
                dynamic_cast<TextureCubeMap*>(level_->GetTextureFromId(texture_id));
            // TODO(anirul): Check the mipmap level (last parameter)!
            frame_buffer_.AttachTexture(opengl_texture->GetId(),
                                        FrameBuffer::GetFrameColorAttachment(i),
                                        FrameBuffer::GetFrameTextureType(texture_frame_), 0);
        } else {
            auto* opengl_texture = dynamic_cast<Texture*>(level_->GetTextureFromId(texture_id));
            // TODO(anirul): Check the mipmap level (last parameter)!
            frame_buffer_.AttachTexture(opengl_texture->GetId(),
                                        FrameBuffer::GetFrameColorAttachment(i),
                                        FrameTextureType::TEXTURE_2D, 0);
        }
        i++;
    }
    frame_buffer_.DrawBuffers(static_cast<std::uint32_t>(texture_out_ids.size()));

    for (const auto& id : material->GetIds()) {
        const auto p  = material->EnableTextureId(id);
        auto* texture = level_->GetTextureFromId(id);
        if (texture->IsCubeMap()) {
            auto* gl_texture = dynamic_cast<TextureCubeMap*>(level_->GetTextureFromId(id));
            assert(gl_texture);
            gl_texture->Bind(p.second);
            program->Uniform(p.first, p.second);
        } else {
            auto* gl_texture = dynamic_cast<Texture*>(level_->GetTextureFromId(id));
            assert(gl_texture);
            gl_texture->Bind(p.second);
            program->Uniform(p.first, p.second);
        }
    }

    auto* gl_static_mesh = dynamic_cast<StaticMesh*>(static_mesh);
    assert(gl_static_mesh);
    glBindVertexArray(gl_static_mesh->GetId());

    auto index_buffer     = level_->GetBufferFromId(static_mesh->GetIndexBufferId());
    auto* gl_index_buffer = dynamic_cast<Buffer*>(index_buffer);
    assert(gl_index_buffer);
    // This was crashing the driver so...
    if (static_mesh->GetIndexSize()) {
        gl_index_buffer->Bind();
        switch (static_mesh->GetRenderPrimitive()) {
            case proto::SceneStaticMesh::TRIANGLE:
                glDrawElements(
                    GL_TRIANGLES,
                    static_cast<GLsizei>(static_mesh->GetIndexSize()) / sizeof(std::uint32_t),
                    GL_UNSIGNED_INT, nullptr);
                break;
            case proto::SceneStaticMesh::POINT:
                glDrawElements(
                    GL_POINTS,
                    static_cast<GLsizei>(static_mesh->GetIndexSize()) / sizeof(std::uint32_t),
                    GL_UNSIGNED_INT, nullptr);
                break;
            case proto::SceneStaticMesh::LINE:
                glDrawElements(
                    GL_LINES,
                    static_cast<GLsizei>(static_mesh->GetIndexSize()) / sizeof(std::uint32_t),
                    GL_UNSIGNED_INT, nullptr);
                break;
            default:
                throw std::runtime_error(fmt::format(
                    "Couldn't draw primitive {}", proto::SceneStaticMesh_RenderPrimitiveEnum_Name(
                                                      static_mesh->GetRenderPrimitive())));
        }
        gl_index_buffer->UnBind();
    }
    program->UnUse();
    glBindVertexArray(0);

    for (const auto id : material->GetIds()) {
        auto* texture = level_->GetTextureFromId(id);
        if (texture->IsCubeMap()) {
            auto* gl_texture = dynamic_cast<TextureCubeMap*>(level_->GetTextureFromId(id));
            assert(gl_texture);
            gl_texture->UnBind();
        } else {
            auto* gl_texture = dynamic_cast<Texture*>(level_->GetTextureFromId(id));
            assert(gl_texture);
            gl_texture->UnBind();
        }
    }
    material->DisableAll();

    if (static_mesh->IsClearBuffer()) {
        glClear(GL_DEPTH_BUFFER_BIT);
    }
}

void Renderer::Display(double dt /* = 0.0*/) {
    auto maybe_quad_id = level_->GetDefaultStaticMeshQuadId();
    if (!maybe_quad_id) throw std::runtime_error("No quad id.");
    auto quad    = level_->GetStaticMeshFromId(maybe_quad_id);
    auto program = level_->GetProgramFromId(display_program_id_);
    UniformWrapper uniform_wrapper(nullptr);
    uniform_wrapper.SetTime(dt);
    program->Use(&uniform_wrapper);
    auto material = level_->GetMaterialFromId(display_material_id_);

    for (const auto id : material->GetIds()) {
        auto* opengl_texture = dynamic_cast<Texture*>(level_->GetTextureFromId(id));
        const auto p         = material->EnableTextureId(id);
        opengl_texture->Bind(p.second);
        program->Uniform(p.first, p.second);
    }

    auto* gl_quad = dynamic_cast<StaticMesh*>(quad);
    assert(gl_quad);
    glBindVertexArray(gl_quad->GetId());

    auto index_buffer     = level_->GetBufferFromId(quad->GetIndexBufferId());
    auto* gl_index_buffer = dynamic_cast<Buffer*>(index_buffer);
    assert(gl_index_buffer);

    gl_index_buffer->Bind();
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(quad->GetIndexSize()) / sizeof(std::int32_t),
                   GL_UNSIGNED_INT, nullptr);
    gl_index_buffer->UnBind();

    program->UnUse();
    glBindVertexArray(0);

    for (const auto id : material->GetIds()) {
        auto* opengl_texture = dynamic_cast<Texture*>(level_->GetTextureFromId(id));
        opengl_texture->UnBind();
    }
    material->DisableAll();
}

void Renderer::SetDepthTest(bool enable) {
    if (enable) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

void Renderer::RenderAllMeshes(double dt /*= 0.0*/) {
    for (const auto& p : level_->GetStaticMeshMaterialIds()) {
        // This should also call clear buffers.
        RenderNode(p.first, p.second, dt);
    }
}

}  // End namespace frame::opengl.
