#include "renderer.h"

#include <GL/glew.h>
#include <cassert>
#include <format>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>

#include "frame/json/parse_uniform.h"
#include "frame/node_matrix.h"
#include "frame/node_static_mesh.h"
#include "frame/opengl/cubemap.h"
#include "frame/opengl/cubemap_views.h"
#include "frame/opengl/file/load_program.h"
#include "frame/opengl/material.h"
#include "frame/opengl/static_mesh.h"
#include "frame/opengl/texture.h"
#include "frame/uniform_collection_wrapper.h"

namespace frame::opengl
{

Renderer::Renderer(LevelInterface& level, glm::uvec4 viewport)
    : level_(level), viewport_(viewport)
{
    frame_buffer_ = std::make_unique<FrameBuffer>();
    render_buffer_ = std::make_unique<RenderBuffer>();
    // TODO(anirul): Check viewport!!!
    render_buffer_->CreateStorage(
        {viewport_.z - viewport_.x, viewport_.w - viewport_.y});
    frame_buffer_->AttachRender(*render_buffer_);
    proto::Program proto_program;
    proto_program.set_name("display");
    proto_program.set_shader_vertex("display.vert");
    proto_program.set_shader_fragment("display.frag");
    auto program = file::LoadProgram(proto_program);
    if (!program)
        throw std::runtime_error("No program!");
    auto material = std::make_unique<Material>();
    program->SetName("DisplayProgram");
    program->SetSerializeEnable(false);
    auto maybe_display_program_id = level_.AddProgram(std::move(program));
    if (!maybe_display_program_id)
        throw std::runtime_error("No display program id.");
    display_program_id_ = maybe_display_program_id;
    material->SetName("DisplayMaterial");
    material->SetSerializeEnable(false);
    auto maybe_display_material_id = level_.AddMaterial(std::move(material));
    if (!maybe_display_material_id)
        throw std::runtime_error("No display material id.");
    display_material_id_ = maybe_display_material_id;
    auto maybe_out_texture_id = level_.GetDefaultOutputTextureId();
    if (!maybe_out_texture_id)
    {
        throw std::runtime_error("No output texture id.");
    }
    auto out_texture_id = maybe_out_texture_id;
    auto& out_texture = level_.GetTextureFromId(out_texture_id);
    // Get material from level as material was moved away.
    level_.GetMaterialFromId(display_material_id_)
        .SetProgramId(display_program_id_);
    if (!level_.GetMaterialFromId(display_material_id_)
             .AddTextureId(out_texture_id, "Display"))
    {
        throw std::runtime_error("Couldn't add texture to material.");
    }
}

std::optional<glm::mat4> Renderer::RenderNode(
    EntityId node_id,
    EntityId material_id,
    const glm::mat4& projection,
    const glm::mat4& view)
{
    // Bail out in case of no node.
    if (node_id == NullId)
        return std::nullopt;
    // Check current node.
    auto& node = level_.GetSceneNodeFromId(node_id);
    // Try to cast to a node static mesh.
    auto& node_static_mesh = dynamic_cast<NodeStaticMesh&>(node);
    auto mesh_id = node.GetLocalMesh();
    // In case no mesh then this is a clear event.
    if (!mesh_id)
    {
        GLbitfield bit_field = 0;
        std::uint32_t clean_buffer = 0;
        for (const auto clean_elem :
             node_static_mesh.GetData().clean_buffer().values())
        {
            clean_buffer |= static_cast<std::uint32_t>(clean_elem);
        }
        if (clean_buffer | proto::CleanBuffer::CLEAR_COLOR)
        {
            bit_field += GL_COLOR_BUFFER_BIT;
        }
        if (clean_buffer | proto::CleanBuffer::CLEAR_DEPTH)
        {
            bit_field += GL_DEPTH_BUFFER_BIT;
        }
        if (bit_field)
        {
            glClear(bit_field);
        }
        return std::nullopt;
    }
    auto& static_mesh = level_.GetStaticMeshFromId(mesh_id);
    // Try to find the material for the mesh.
    if (material_id == NullId)
    {
        throw std::runtime_error("No material?");
    }
    MaterialInterface& material = level_.GetMaterialFromId(material_id);
    glm::mat4 model = node.GetLocalModel(delta_time_);
    RenderMesh(static_mesh, material, projection, view, model);
    return model;
}

void Renderer::RenderMesh(
    StaticMeshInterface& static_mesh,
    MaterialInterface& material,
    const glm::mat4& projection,
    const glm::mat4& view,
    const glm::mat4& model /* = glm::mat4(1.0f)*/)
{
    auto program_id = material.GetProgramId();
    auto& program = level_.GetProgramFromId(program_id);
    assert(program.GetOutputTextureIds().size());

    // In case the camera doesn't exist it will create a basic one.
    UniformCollectionWrapper uniform_collection_wrapper(
        projection, view, model, delta_time_);
    if (level_.GetLights().size() > 0)
    {
        auto& light = level_.GetLightFromId(level_.GetLights()[0]);
        uniform_collection_wrapper.AddUniform(
            std::make_unique<Uniform>("light_pos", light.GetVector()));
        uniform_collection_wrapper.AddUniform(
            std::make_unique<Uniform>("light_color", light.GetColorIntensity()));
    }
    auto& camera = level_.GetCameraFromId(level_.GetDefaultCameraId());
    uniform_collection_wrapper.AddUniform(
        std::make_unique<Uniform>("camera_pos", camera.GetPosition()));
    if (render_time_ == proto::NodeStaticMesh::SCENE_RENDER_TIME)
    {
        std::unique_ptr<UniformInterface> env_map_uniform =
            std::make_unique<Uniform>("env_map_model", env_map_model_);
        uniform_collection_wrapper.AddUniform(std::move(env_map_uniform));
    }
    // Go through the callback.
    callback_(uniform_collection_wrapper, static_mesh, material);
    program.Use(uniform_collection_wrapper, &level_);

    auto texture_out_ids = program.GetOutputTextureIds();
    auto& texture_ref = level_.GetTextureFromId(*texture_out_ids.cbegin());
    auto size = json::ParseSize(texture_ref.GetData().size());

    glViewport(viewport_.x, viewport_.y, viewport_.z, viewport_.w);

    ScopedBind scoped_frame(*frame_buffer_);
    int i = 0;
    for (const auto& texture_id : program.GetOutputTextureIds())
    {
        if (level_.GetTextureFromId(texture_id).GetData().cubemap())
        {
            auto& opengl_texture =
                dynamic_cast<Cubemap&>(level_.GetTextureFromId(texture_id));
            // TODO(anirul): Check the mipmap level (last parameter)!
            frame_buffer_->AttachTexture(
                opengl_texture.GetId(),
                FrameBuffer::GetFrameColorAttachment(i),
                FrameBuffer::GetFrameTextureType(texture_frame_),
                0);
        }
        else
        {
            auto& opengl_texture =
                dynamic_cast<Texture&>(level_.GetTextureFromId(texture_id));
            // TODO(anirul): Check the mipmap level (last parameter)!
            frame_buffer_->AttachTexture(
                opengl_texture.GetId(),
                FrameBuffer::GetFrameColorAttachment(i),
                FrameTextureType::TEXTURE_2D,
                0);
        }
        i++;
    }
    frame_buffer_->DrawBuffers(
        static_cast<std::uint32_t>(texture_out_ids.size()));

    std::map<std::string, std::vector<std::int32_t>> uniform_include;
    for (const auto& id : material.GetTextureIds())
    {
        EntityId texture_id = NullId;
        if (level_.GetEnumTypeFromId(id) == EntityTypeEnum::TEXTURE)
        {
            texture_id = id;
        }
        else
        {
            // TODO(anirul): Find a better way to find the texture
            // associated with the stream.
            texture_id = id + 1;
        }
        // TODO(anirul): Why? id and not texture id?
        const auto p = material.EnableTextureId(id);
        auto& texture = level_.GetTextureFromId(texture_id);
        if (texture.GetData().cubemap())
        {
            auto& gl_texture =
                dynamic_cast<Cubemap&>(level_.GetTextureFromId(texture_id));
            gl_texture.Bind(p.second);
        }
        else
        {
            auto& gl_texture =
                dynamic_cast<Texture&>(level_.GetTextureFromId(texture_id));
            gl_texture.Bind(p.second);
        }
        std::unique_ptr<UniformInterface> uniform_interface =
            std::make_unique<Uniform>(p.first, p.second);
        program.AddUniform(std::move(uniform_interface));
    }
    int j = 0;
    for (const auto& name : material.GetBufferNames())
    {
        auto id = level_.GetIdFromName(name);
        if (id == NullId)
        {
            throw std::runtime_error("Could not find buffer: " + name);
        }
        auto& buffer = dynamic_cast<opengl::Buffer&>(level_.GetBufferFromId(id));
        auto inner_name = material.GetInnerBufferName(name);
        dynamic_cast<opengl::Program&>(program).AddBuffer(id, inner_name, j++);
    }

    auto& gl_static_mesh = dynamic_cast<StaticMesh&>(static_mesh);
    glBindVertexArray(gl_static_mesh.GetId());

    auto& index_buffer = level_.GetBufferFromId(static_mesh.GetIndexBufferId());
    auto& gl_index_buffer = dynamic_cast<Buffer&>(index_buffer);
    // This was crashing the driver so...
    if (static_mesh.GetIndexSize())
    {
        gl_index_buffer.Bind();
        switch (static_mesh.GetData().render_primitive_enum())
        {
        case proto::NodeStaticMesh::TRIANGLE_PRIMITIVE:
            glDrawElements(
                GL_TRIANGLES,
                static_cast<GLsizei>(static_mesh.GetIndexSize()) /
                    sizeof(std::uint32_t),
                GL_UNSIGNED_INT,
                nullptr);
            break;
        case proto::NodeStaticMesh::POINT_PRIMITIVE:
            glDrawElements(
                GL_POINTS,
                static_cast<GLsizei>(static_mesh.GetIndexSize()) /
                    sizeof(std::uint32_t),
                GL_UNSIGNED_INT,
                nullptr);
            break;
        case proto::NodeStaticMesh::LINE_PRIMITIVE:
            glDrawElements(
                GL_LINES,
                static_cast<GLsizei>(static_mesh.GetIndexSize()) /
                    sizeof(std::uint32_t),
                GL_UNSIGNED_INT,
                nullptr);
            break;
        default:
            throw std::runtime_error(
                std::format(
                    "Couldn't draw primitive {}",
                    proto::NodeStaticMesh_RenderPrimitiveEnum_Name(
                        static_mesh.GetData().render_primitive_enum())));
        }
        gl_index_buffer.UnBind();
    }
    program.UnUse();
    glBindVertexArray(0);

    for (const auto id : material.GetTextureIds())
    {
        EntityId texture_id = id;
        if (level_.GetEnumTypeFromId(id) != EntityTypeEnum::TEXTURE)
        {
            texture_id = id + 1;
        }
        auto& texture = level_.GetTextureFromId(texture_id);
        if (texture.GetData().cubemap())
        {
            auto& gl_texture =
                dynamic_cast<Cubemap&>(level_.GetTextureFromId(texture_id));
            gl_texture.UnBind();
        }
        else
        {
            auto& gl_texture =
                dynamic_cast<Texture&>(level_.GetTextureFromId(texture_id));
            gl_texture.UnBind();
        }
    }
    material.DisableAll();

    if (static_mesh.IsClearBuffer())
    {
        glClear(GL_DEPTH_BUFFER_BIT);
    }
}

void Renderer::PresentFinal()
{
    auto maybe_quad_id = level_.GetDefaultStaticMeshQuadId();
    if (maybe_quad_id == NullId)
        throw std::runtime_error("No quad id.");
    auto& quad = level_.GetStaticMeshFromId(maybe_quad_id);
    auto& program = level_.GetProgramFromId(display_program_id_);
    UniformCollectionWrapper uniform_collection_wrapper{};
    program.Use(uniform_collection_wrapper, &level_);
    auto& material = level_.GetMaterialFromId(display_material_id_);
    for (const auto id : material.GetTextureIds())
    {
        auto& opengl_texture =
            dynamic_cast<Texture&>(level_.GetTextureFromId(id));
        const auto p = material.EnableTextureId(id);
        opengl_texture.Bind(p.second);
        std::unique_ptr<UniformInterface> uniform_interface =
            std::make_unique<Uniform>(p.first, p.second);
        program.AddUniform(std::move(uniform_interface));
    }
    auto& gl_quad = dynamic_cast<StaticMesh&>(quad);
    glBindVertexArray(gl_quad.GetId());
    auto& index_buffer = level_.GetBufferFromId(quad.GetIndexBufferId());
    auto& gl_index_buffer = dynamic_cast<Buffer&>(index_buffer);

    gl_index_buffer.Bind();
    glDrawElements(
        GL_TRIANGLES,
        static_cast<GLsizei>(quad.GetIndexSize()) / sizeof(std::int32_t),
        GL_UNSIGNED_INT,
        nullptr);
    gl_index_buffer.UnBind();

    program.UnUse();
    glBindVertexArray(0);

    for (const auto id : material.GetTextureIds())
    {
        auto& opengl_texture =
            dynamic_cast<Texture&>(level_.GetTextureFromId(id));
        opengl_texture.UnBind();
    }
    material.DisableAll();
}

void Renderer::SetDepthTest(bool enable)
{
    if (enable)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
}

void Renderer::PreRender()
{
    render_time_ = proto::NodeStaticMesh::PRE_RENDER_TIME;
    // This will ensure that it is only true once.
    auto first_render = std::exchange(first_render_, false);
    for (const auto& p : level_.GetStaticMeshMaterialIds(
             proto::NodeStaticMesh::PRE_RENDER_TIME))
    {
        if (first_render)
        {
            auto material_id = p.second;
            auto temp_viewport = viewport_;
            // Now this get the image size from the environment map.
            auto& material = level_.GetMaterialFromId(material_id);
            auto ids = material.GetTextureIds();
            assert(!ids.empty());
            auto& texture = level_.GetTextureFromId(ids[0]);
            auto size = json::ParseSize(texture.GetData().size());
            viewport_ = glm::ivec4(0, 0, size.x / 2, size.y / 2);
            for (std::uint32_t i = 0; i < 6; ++i)
            {
                proto::TextureFrame texture_frame;
                texture_frame.set_value(
                    static_cast<proto::TextureFrame::Enum>(
                        proto::TextureFrame::CUBE_MAP_POSITIVE_X + i));
                SetCubeMapTarget(texture_frame);
                RenderNode(
                    p.first, material_id, kProjectionCubemap, kViewsCubemap[i]);
            }
            {
                // Again why?
                proto::TextureFrame texture_frame;
                texture_frame.set_value(
                    static_cast<proto::TextureFrame::Enum>(
                        proto::TextureFrame::CUBE_MAP_POSITIVE_X));
                SetCubeMapTarget(texture_frame);
                RenderNode(
                    p.first, material_id, kProjectionCubemap, kViewsCubemap[0]);
            }
            viewport_ = temp_viewport;
        }
    }
}

void Renderer::RenderSkybox(const CameraInterface& camera)
{
    render_time_ = proto::NodeStaticMesh::SKYBOX_RENDER_TIME;
    for (const auto& p : level_.GetStaticMeshMaterialIds(
             proto::NodeStaticMesh::SKYBOX_RENDER_TIME))
    {
        auto maybe_model = RenderNode(
            p.first,
            p.second,
            camera.ComputeProjection(),
            camera.ComputeView());
        if (maybe_model)
        {
            env_map_model_ = *maybe_model;
        }
    }
}

void Renderer::RenderScene(const CameraInterface& camera)
{
    render_time_ = proto::NodeStaticMesh::SCENE_RENDER_TIME;
    for (const auto& p : level_.GetStaticMeshMaterialIds(
             proto::NodeStaticMesh::SCENE_RENDER_TIME))
    {
        RenderNode(
            p.first,
            p.second,
            camera.ComputeProjection(),
            camera.ComputeView());
    }
}

void Renderer::PostProcess()
{
    render_time_ = proto::NodeStaticMesh::POST_PROCESS_TIME;
    for (const auto& p : level_.GetStaticMeshMaterialIds(
             proto::NodeStaticMesh::POST_PROCESS_TIME))
    {
        // Is it correct for projection and view? This is a post process?
        RenderNode(p.first, p.second, glm::mat4(1.0), glm::mat4(1.0));
    }
}

} // End namespace frame::opengl.
