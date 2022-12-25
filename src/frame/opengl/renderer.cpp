#include "renderer.h"

#include <GL/glew.h>
#include <fmt/core.h>

#include <stdexcept>

#include "frame/context.h"
#include "frame/node_matrix.h"
#include "frame/node_static_mesh.h"
#include "frame/opengl/file/load_program.h"
#include "frame/opengl/material.h"
#include "frame/opengl/static_mesh.h"
#include "frame/opengl/texture.h"
#include "frame/opengl/texture_cube_map.h"
#include "frame/uniform_wrapper.h"

namespace frame::opengl {

Renderer::Renderer(Context context, glm::uvec4 viewport)
    : device_(context.device), level_(context.level), viewport_(viewport) {
    // TODO(anirul): Check viewport!!!
    render_buffer_.CreateStorage({ viewport_.z - viewport_.x, viewport_.w - viewport_.y });
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
    auto maybe_out_texture_id = level_->GetDefaultOutputTextureId();
    if (!maybe_out_texture_id) throw std::runtime_error("No output texture id.");
    auto out_texture_id = maybe_out_texture_id;
    // Get material from level as material was moved away.
    level_->GetMaterialFromId(display_material_id_)->SetProgramId(display_program_id_);
    if (!level_->GetMaterialFromId(display_material_id_)->AddTextureId(out_texture_id, "Display")) {
        throw std::runtime_error("Couldn't add texture to material.");
    }
}

void Renderer::RenderNode(EntityId node_id, EntityId material_id, const glm::mat4& projection,
                          const glm::mat4& view, double dt /* = 0.0*/) {
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
    RenderMesh(static_mesh, material, projection, view, node->GetLocalModel(dt), dt);
}

void Renderer::RenderMesh(StaticMeshInterface* static_mesh, MaterialInterface* material,
                          const glm::mat4& projection, const glm::mat4& view,
                          const glm::mat4& model /* = glm::mat4(1.0f)*/, double dt /* = 0.0*/) {
    if (!static_mesh) throw std::runtime_error("StaticMesh ptr doesn't exist.");
    if (!material) throw std::runtime_error("No material!");
    auto program_id = material->GetProgramId();
    auto program    = level_->GetProgramFromId(program_id);
    if (!program) throw std::runtime_error("Program ptr doesn't exist.");
    last_program_id_ = program_id;
    assert(program->GetOutputTextureIds().size());

    // In case the camera doesn't exist it will create a basic one.
    UniformWrapper uniform_wrapper(projection, view, model, dt);

    // Go through the plugin list.
    assert(device_);
    for (auto* plugin : device_->GetPluginPtrs()) {
        plugin->PreRender(uniform_wrapper, device_, static_mesh, material);
    }
    program->Use(&uniform_wrapper);

    auto texture_out_ids = program->GetOutputTextureIds();

    glViewport(viewport_.x, viewport_.y, viewport_.z, viewport_.w);

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

    std::map<std::string, std::vector<std::int32_t>> uniform_include;
    for (const auto& id : material->GetIds()) {
        EntityId texture_id = NullId;
        if (level_->GetEnumTypeFromId(id) == EntityTypeEnum::TEXTURE) {
            texture_id = id;
        } else {
            // TODO(anirul): Find a better way to find the texture associated with the stream.
            texture_id = id + 1;
        }
        // TODO(anirul): Why? id and not texture id?
        const auto p  = material->EnableTextureId(id);
        auto* texture = level_->GetTextureFromId(texture_id);
        if (texture->IsCubeMap()) {
            auto* gl_texture = dynamic_cast<TextureCubeMap*>(level_->GetTextureFromId(texture_id));
            assert(gl_texture);
            gl_texture->Bind(p.second);
            program->Uniform(p.first, p.second);
        } else {
            auto* gl_texture = dynamic_cast<Texture*>(level_->GetTextureFromId(texture_id));
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
        EntityId texture_id = id;
        if (level_->GetEnumTypeFromId(id) != EntityTypeEnum::TEXTURE) {
            texture_id = id + 1;
        }
        auto* texture = level_->GetTextureFromId(texture_id);
        if (texture->IsCubeMap()) {
            auto* gl_texture = dynamic_cast<TextureCubeMap*>(level_->GetTextureFromId(texture_id));
            assert(gl_texture);
            gl_texture->UnBind();
        } else {
            auto* gl_texture = dynamic_cast<Texture*>(level_->GetTextureFromId(texture_id));
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
    UniformWrapper uniform_wrapper{};
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

void Renderer::RenderAllMeshes(const glm::mat4& projection, const glm::mat4& view,
                               double dt /*= 0.0*/) {
    for (const auto& p : level_->GetStaticMeshMaterialIds()) {
        // This should also call clear buffers.
        RenderNode(p.first, p.second, projection, view, dt);
    }
}

}  // End namespace frame::opengl.
