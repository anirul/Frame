#include "renderer.h"

#include <cassert>
#include <format>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>

#include "frame/json/parse_uniform.h"
#include "frame/json/program_catalog.h"
#include "frame/node_matrix.h"
#include "frame/node_mesh.h"
#include "frame/opengl/cubemap.h"
#include "frame/opengl/cubemap_views.h"
#include "frame/opengl/file/load_program.h"
#include "frame/opengl/material.h"
#include "frame/opengl/mesh.h"
#include "frame/opengl/skinned_mesh.h"
#include "frame/opengl/texture.h"
#include "frame/uniform_collection_wrapper.h"

namespace frame::opengl
{

namespace
{

bool IsRaytracingProgram(const ProgramInterface& program)
{
    const auto key = frame::json::ResolveProgramKey(program.GetData());
    return frame::json::IsRaytracingProgramKey(key);
}

} // namespace

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
    proto_program.set_pipeline_name("display");
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

void Renderer::UpdateRaytraceBuffersIfNeeded(SkinnedMesh& skinned_mesh)
{
    const double skinning_time = skinned_mesh.GetSkinningTime(delta_time_);

    if (skinned_mesh.HasRaytraceTriangleCallback())
    {
        const EntityId triangle_buffer_id = skinned_mesh.GetTriangleBufferId();
        if (triangle_buffer_id)
        {
            auto triangles =
                skinned_mesh.EvaluateRaytraceTriangles(skinning_time);
            if (!triangles.empty())
            {
                auto& triangle_buffer = dynamic_cast<Buffer&>(
                    level_.GetBufferFromId(triangle_buffer_id));
                triangle_buffer.Copy(triangles);
            }
        }
    }

    if (skinned_mesh.HasRaytraceBvhCallback())
    {
        const EntityId bvh_buffer_id = skinned_mesh.GetBvhBufferId();
        if (bvh_buffer_id)
        {
            auto bvh_nodes = skinned_mesh.EvaluateRaytraceBvh(skinning_time);
            if (!bvh_nodes.empty())
            {
                auto& bvh_buffer =
                    dynamic_cast<Buffer&>(level_.GetBufferFromId(bvh_buffer_id));
                bvh_buffer.Copy(
                    bvh_nodes.size() * sizeof(frame::BVHNode),
                    bvh_nodes.data());
            }
        }
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
    // Try to cast to a node Mesh.
    auto& node_mesh = dynamic_cast<NodeMesh&>(node);
    auto mesh_id = node.GetLocalMesh();
    // In case no mesh then this is a clear event.
    if (!mesh_id)
    {
        GLbitfield bit_field = 0;
        std::uint32_t clean_buffer = 0;
        for (const auto clean_elem :
             node_mesh.GetData().clean_buffer().values())
        {
            clean_buffer |= static_cast<std::uint32_t>(clean_elem);
        }
        if (clean_buffer & proto::CleanBuffer::CLEAR_COLOR)
        {
            bit_field |= GL_COLOR_BUFFER_BIT;
        }
        if (clean_buffer & proto::CleanBuffer::CLEAR_DEPTH)
        {
            bit_field |= GL_DEPTH_BUFFER_BIT;
        }
        if (bit_field)
        {
            glClear(bit_field);
        }
        return std::nullopt;
    }
    auto& mesh = level_.GetMeshFromId(mesh_id);
    // Try to find the material for the mesh.
    if (material_id == NullId)
    {
        throw std::runtime_error("No material?");
    }
    MaterialInterface& material = level_.GetMaterialFromId(material_id);
    EntityId program_id = material.GetProgramId(&level_);
    if (!program_id)
    {
        program_id = level_.GetRenderPassProgramId(
            node_mesh.GetData().render_time_enum());
        if (program_id)
        {
            material.SetProgramId(program_id);
        }
    }
    if (!program_id)
    {
        throw std::runtime_error("No program configured for material.");
    }
    auto& program = level_.GetProgramFromId(program_id);
    glm::mat4 model = node.GetLocalModel(delta_time_);
    MeshInterface* mesh_to_render = &mesh;
    const EntityId quad_id = level_.GetDefaultMeshQuadId();
    if (quad_id != NullId &&
        mesh_id != quad_id &&
        IsRaytracingProgram(program))
    {
        if (auto* gl_skinned_mesh = dynamic_cast<SkinnedMesh*>(&mesh))
        {
            UpdateRaytraceBuffersIfNeeded(*gl_skinned_mesh);
        }
        mesh_to_render = &level_.GetMeshFromId(quad_id);
    }
    RenderMesh(*mesh_to_render, material, projection, view, model);
    return model;
}

void Renderer::RenderMesh(
    MeshInterface& mesh,
    MaterialInterface& material,
    const glm::mat4& projection,
    const glm::mat4& view,
    const glm::mat4& model /* = glm::mat4(1.0f)*/)
{
    auto program_id = material.GetProgramId();
    auto& program = level_.GetProgramFromId(program_id);
    glm::mat4 model_matrix = model;
    if (!program.GetTemporarySceneRoot().empty())
    {
        auto temp_id = level_.GetIdFromName(program.GetTemporarySceneRoot());
        if (temp_id != NullId)
        {
            auto& temp_node = level_.GetSceneNodeFromId(temp_id);
            model_matrix = temp_node.GetLocalModel(delta_time_);
        }
    }

    // In case the camera doesn't exist it will create a basic one.
    UniformCollectionWrapper uniform_collection_wrapper(
        projection, view, model_matrix, delta_time_);
    if (level_.GetLights().size() > 0)
    {
        auto& light = level_.GetLightFromId(level_.GetLights()[0]);
        uniform_collection_wrapper.AddUniform(
            std::make_unique<Uniform>("light_dir", light.GetVector()));
        uniform_collection_wrapper.AddUniform(
            std::make_unique<Uniform>(
                "light_color", light.GetColorIntensity()));
    }
    if (render_time_ == proto::NodeMesh::SCENE_RENDER_TIME)
    {
        std::unique_ptr<UniformInterface> env_map_uniform =
            std::make_unique<Uniform>("env_map_model", env_map_model_);
        uniform_collection_wrapper.AddUniform(std::move(env_map_uniform));
    }
    // Go through the callback.
    callback_(uniform_collection_wrapper, mesh, material);

    // Add node-based model matrices.
    for (const auto& name : material.GetNodeNames())
    {
        auto node_id = level_.GetIdFromName(name);
        if (node_id == NullId)
        {
            throw std::runtime_error("Could not find node: " + name);
        }
        auto& node = level_.GetSceneNodeFromId(node_id);
        glm::mat4 node_model = node.GetLocalModel(delta_time_);
        auto inner_name = material.GetInnerNodeName(name);
        std::unique_ptr<UniformInterface> node_uniform =
            std::make_unique<Uniform>(inner_name, node_model);
        uniform_collection_wrapper.AddUniform(std::move(node_uniform));
    }

    // Register shader storage buffers before using the program so they are
    // bound when Program::Use uploads them.
    int j = 0;
    for (const auto& name : material.GetBufferNames())
    {
        auto id = level_.GetIdFromName(name);
        if (id == NullId)
        {
            throw std::runtime_error("Could not find buffer: " + name);
        }
        auto inner_name = material.GetInnerBufferName(name);
        dynamic_cast<opengl::Program&>(program).AddBuffer(id, inner_name, j++);
    }

    auto& gl_mesh = dynamic_cast<Mesh&>(mesh);
    auto* gl_skinned_mesh = dynamic_cast<SkinnedMesh*>(&gl_mesh);
    if (gl_skinned_mesh)
    {
        UpdateRaytraceBuffersIfNeeded(*gl_skinned_mesh);
    }
    program.Use(uniform_collection_wrapper, &level_);
    int skinning_enabled = 0;
    auto& gl_program = dynamic_cast<opengl::Program&>(program);
    if (gl_skinned_mesh && gl_skinned_mesh->HasSkinning())
    {
        const double skinning_time =
            gl_skinned_mesh->GetSkinningTime(delta_time_);
        auto bone_matrices = gl_skinned_mesh->EvaluateSkinning(skinning_time);
        if (!bone_matrices.empty())
        {
            constexpr std::size_t kMaxBones = 128;
            if (bone_matrices.size() > kMaxBones)
            {
                bone_matrices.resize(kMaxBones);
            }
            gl_program.UploadMatrix4ArrayUniform(
                "bone_matrices", bone_matrices);
            skinning_enabled = 1;
        }
    }
    if (program.HasUniform("skinning_enabled"))
    {
        program.AddUniform(
            std::make_unique<Uniform>("skinning_enabled", skinning_enabled));
    }

    auto texture_out_ids = program.GetOutputTextureIds();
    glViewport(viewport_.x, viewport_.y, viewport_.z, viewport_.w);
    std::unique_ptr<ScopedBind> scoped_frame;
    if (!texture_out_ids.empty())
    {
        scoped_frame = std::make_unique<ScopedBind>(*frame_buffer_);
        int i = 0;
        for (const auto& texture_id : texture_out_ids)
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
    }
    else
    {
        scoped_frame = std::make_unique<ScopedBind>(*frame_buffer_);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }

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

    glBindVertexArray(gl_mesh.GetId());

    auto& index_buffer = level_.GetBufferFromId(mesh.GetIndexBufferId());
    auto& gl_index_buffer = dynamic_cast<Buffer&>(index_buffer);
    // This was crashing the driver so...
    if (mesh.GetIndexSize())
    {
        gl_index_buffer.Bind();
        switch (mesh.GetData().render_primitive_enum())
        {
        case proto::NodeMesh::TRIANGLE_PRIMITIVE:
            glDrawElements(
                GL_TRIANGLES,
                static_cast<GLsizei>(mesh.GetIndexSize()) /
                    sizeof(std::uint32_t),
                GL_UNSIGNED_INT,
                nullptr);
            break;
        case proto::NodeMesh::POINT_PRIMITIVE:
            glDrawElements(
                GL_POINTS,
                static_cast<GLsizei>(mesh.GetIndexSize()) /
                    sizeof(std::uint32_t),
                GL_UNSIGNED_INT,
                nullptr);
            break;
        case proto::NodeMesh::LINE_PRIMITIVE:
            glDrawElements(
                GL_LINES,
                static_cast<GLsizei>(mesh.GetIndexSize()) /
                    sizeof(std::uint32_t),
                GL_UNSIGNED_INT,
                nullptr);
            break;
        default:
            throw std::runtime_error(
                std::format(
                    "Couldn't draw primitive {}",
                    proto::NodeMesh_RenderPrimitiveEnum_Name(
                        mesh.GetData().render_primitive_enum())));
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

    if (mesh.IsClearBuffer())
    {
        glClear(GL_DEPTH_BUFFER_BIT);
    }
}

void Renderer::PresentFinal()
{
    auto maybe_quad_id = level_.GetDefaultMeshQuadId();
    if (maybe_quad_id == NullId)
        throw std::runtime_error("No quad id.");
    auto& quad = level_.GetMeshFromId(maybe_quad_id);
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
    auto& gl_quad = dynamic_cast<Mesh&>(quad);
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
    render_time_ = proto::NodeMesh::PRE_RENDER_TIME;
    // This will ensure that it is only true once.
    auto first_render = std::exchange(first_render_, false);
    for (const auto& p : level_.GetMeshMaterialIds(
             proto::NodeMesh::PRE_RENDER_TIME))
    {
        auto& node = level_.GetSceneNodeFromId(p.first);
        if (node.GetLocalMesh())
        {
            auto& mesh = level_.GetMeshFromId(node.GetLocalMesh());
            auto* gl_skinned_mesh =
                dynamic_cast<SkinnedMesh*>(&mesh);
            if (gl_skinned_mesh)
            {
                UpdateRaytraceBuffersIfNeeded(*gl_skinned_mesh);
            }
        }
        if (first_render)
        {
            auto material_id = p.second;
            auto temp_viewport = viewport_;
            // Query textures from the material.
            auto& material = level_.GetMaterialFromId(material_id);
            if (material.GetPreprocessProgramId())
            {
                auto saved_program = material.GetProgramId();
                auto preprocess_id = material.GetPreprocessProgramId();
                if (preprocess_id)
                {
                    auto& preprocess_program =
                        level_.GetProgramFromId(preprocess_id);
                    auto out_ids = preprocess_program.GetOutputTextureIds();
                    if (out_ids.empty())
                    {
                        texture_frame_.set_value(proto::TextureFrame::TEXTURE_2D);
                        material.SetProgramId(preprocess_id);
                        RenderNode(
                            p.first,
                            material_id,
                            kProjectionCubemap,
                            kViewsCubemap[0]);
                        material.SetProgramId(saved_program);
                        viewport_ = temp_viewport;
                        continue;
                    }
                    auto& tex = level_.GetTextureFromId(*out_ids.begin());
                    auto size = json::ParseSize(tex.GetData().size());
                    viewport_ = glm::ivec4(0, 0, size.x, size.y);
                    material.SetProgramId(preprocess_id);
                    RenderNode(
                        p.first,
                        material_id,
                        kProjectionCubemap,
                        kViewsCubemap[0]);
                    material.SetProgramId(saved_program);
                    viewport_ = temp_viewport;
                }
                continue;
            }
            auto ids = material.GetTextureIds();
            if (ids.empty())
            {
                // Mesh has no target texture: just render once to populate
                // buffers without touching the framebuffer.
                RenderNode(
                    p.first, material_id, kProjectionCubemap, kViewsCubemap[0]);
                continue;
            }
            auto& texture = level_.GetTextureFromId(ids[0]);
            auto size = json::ParseSize(texture.GetData().size());
            viewport_ = glm::ivec4(0, 0, size.x, size.y);
            if (texture.GetData().cubemap())
            {
                for (std::uint32_t i = 0; i < 6; ++i)
                {
                    proto::TextureFrame texture_frame;
                    texture_frame.set_value(
                        static_cast<proto::TextureFrame::Enum>(
                            proto::TextureFrame::CUBE_MAP_POSITIVE_X + i));
                    SetCubeMapTarget(texture_frame);
                    RenderNode(
                        p.first,
                        material_id,
                        kProjectionCubemap,
                        kViewsCubemap[i]);
                }
            }
            else
            {
                // Regular 2D texture target.
                texture_frame_.set_value(proto::TextureFrame::TEXTURE_2D);
                RenderNode(
                    p.first, material_id, kProjectionCubemap, kViewsCubemap[0]);
            }
            viewport_ = temp_viewport;
        }
    }
}

void Renderer::RenderSkybox(const CameraInterface& camera)
{
    render_time_ = proto::NodeMesh::SKYBOX_RENDER_TIME;
    for (const auto& p : level_.GetMeshMaterialIds(
             proto::NodeMesh::SKYBOX_RENDER_TIME))
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
    render_time_ = proto::NodeMesh::SCENE_RENDER_TIME;
    for (const auto& p : level_.GetMeshMaterialIds(
             proto::NodeMesh::SCENE_RENDER_TIME))
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
    render_time_ = proto::NodeMesh::POST_PROCESS_TIME;
    for (const auto& p : level_.GetMeshMaterialIds(
             proto::NodeMesh::POST_PROCESS_TIME))
    {
        // Is it correct for projection and view? This is a post process?
        RenderNode(p.first, p.second, glm::mat4(1.0), glm::mat4(1.0));
    }
}

} // End namespace frame::opengl.



