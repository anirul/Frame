#include "frame/json/level_data.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <format>
#include <numeric>
#include <optional>
#include <stdexcept>

namespace frame::json
{

namespace
{

enum class SupportedScenePreset
{
    Unknown,
    Raster,
    Cubemap,
    Raytrace,
};

std::string ToLowerAscii(std::string value)
{
    std::transform(
        value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
    return value;
}

proto::Uniform MakeUniformEnum(
    const std::string& name, proto::Uniform::UniformEnum uniform_enum)
{
    proto::Uniform uniform;
    uniform.set_name(name);
    uniform.set_uniform_enum(uniform_enum);
    return uniform;
}

proto::ProgramBinding MakeBinding(
    const std::string& name,
    std::uint32_t binding,
    proto::ProgramBinding::BindingType binding_type,
    std::initializer_list<proto::ProgramStage> stages)
{
    proto::ProgramBinding binding_info;
    binding_info.set_name(name);
    binding_info.set_binding(binding);
    binding_info.set_binding_type(binding_type);
    for (const auto stage : stages)
    {
        binding_info.add_stages(stage);
    }
    return binding_info;
}

ProgramInfo MakeProgramInfo(
    proto::Program proto_program,
    ShaderFiles opengl_files,
    ShaderFiles vulkan_files)
{
    ProgramInfo info;
    info.name = proto_program.name();
    info.proto = std::move(proto_program);
    info.opengl = std::move(opengl_files);
    info.vulkan = std::move(vulkan_files);
    return info;
}

ProgramInfo MakeCubemapProgram(const std::string& output_texture_name)
{
    proto::Program program;
    program.set_name("CubemapProgram");
    program.set_pipeline_name("cubemap");
    program.add_input_texture_names("skybox");
    program.add_output_texture_names(output_texture_name);
    program.mutable_input_scene_type()->set_value(proto::SceneType::CUBE);
    *program.add_bindings() = MakeBinding(
        "Skybox",
        0,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::FRAGMENT});
    *program.add_uniforms() =
        MakeUniformEnum("projection", proto::Uniform::PROJECTION_MAT4);
    *program.add_uniforms() =
        MakeUniformEnum("view", proto::Uniform::VIEW_MAT4);
    *program.add_uniforms() =
        MakeUniformEnum("model", proto::Uniform::MODEL_MAT4);
    *program.add_uniforms() =
        MakeUniformEnum("time_s", proto::Uniform::FLOAT_TIME_S);
    return MakeProgramInfo(
        std::move(program),
        {.vertex_shader = "cubemap.vert", .fragment_shader = "cubemap.frag"},
        {.vertex_shader = "cubemap.vert", .fragment_shader = "cubemap.frag"});
}

void AddSharedRaytracingUniforms(proto::Program& program)
{
    *program.add_uniforms() =
        MakeUniformEnum("projection", proto::Uniform::PROJECTION_MAT4);
    *program.add_uniforms() =
        MakeUniformEnum("view", proto::Uniform::VIEW_MAT4);
    *program.add_uniforms() =
        MakeUniformEnum("projection_inv", proto::Uniform::PROJECTION_INV_MAT4);
    *program.add_uniforms() =
        MakeUniformEnum("view_inv", proto::Uniform::VIEW_INV_MAT4);
    *program.add_uniforms() =
        MakeUniformEnum("model", proto::Uniform::MODEL_MAT4);
    *program.add_uniforms() =
        MakeUniformEnum("model_inv", proto::Uniform::MODEL_INV_MAT4);
    *program.add_uniforms() = MakeUniformEnum(
        "camera_position", proto::Uniform::CAMERA_POSITION_VEC3);
    *program.add_uniforms() =
        MakeUniformEnum("light_dir", proto::Uniform::LIGHT_POSITION_VEC3);
    *program.add_uniforms() =
        MakeUniformEnum("light_color", proto::Uniform::LIGHT_COLOR_VEC3);
}

void AddSharedRaytracingInputs(
    proto::Program& program, const std::string& output_texture_name)
{
    program.add_output_texture_names(output_texture_name);
    program.add_input_texture_names("skybox");
    program.add_input_texture_names("skybox_env");
    program.mutable_input_scene_type()->set_value(proto::SceneType::QUAD);
    program.set_input_scene_root_name("mesh_holder");
}

void AddRaytraceSceneMaterialInputs(proto::Program& program)
{
    program.add_input_texture_names("transmissive_albedo_texture");
    program.add_input_texture_names("transmissive_normal_texture");
    program.add_input_texture_names("transmissive_roughness_texture");
    program.add_input_texture_names("transmissive_metallic_texture");
    program.add_input_texture_names("transmissive_ao_texture");
    program.add_input_texture_names("transmissive_transmission_texture");
    program.add_input_texture_names("transmissive_ior_texture");
    program.add_input_texture_names("transmissive_thickness_texture");
    program.add_input_texture_names("transmissive_attenuation_color_texture");
    program.add_input_texture_names(
        "transmissive_attenuation_distance_texture");
    program.add_input_texture_names("opaque_albedo_texture");
    program.add_input_texture_names("opaque_normal_texture");
    program.add_input_texture_names("opaque_roughness_texture");
    program.add_input_texture_names("opaque_metallic_texture");
    program.add_input_texture_names("opaque_ao_texture");
    program.add_input_texture_names("opaque_specular_factor_texture");
    program.add_input_texture_names("opaque_specular_color_texture");
}

ProgramInfo MakeRaytraceProgram(const std::string& output_texture_name)
{
    proto::Program program;
    program.set_name("RayTraceProgram");
    program.set_pipeline_name("raytrace");
    AddSharedRaytracingInputs(program, output_texture_name);
    AddRaytraceSceneMaterialInputs(program);
    *program.add_bindings() = MakeBinding(
        "output_image",
        0,
        proto::ProgramBinding::STORAGE_IMAGE,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "raytrace_output",
        1,
        proto::ProgramBinding::OUTPUT_SAMPLER,
        {proto::ProgramStage::FRAGMENT});
    *program.add_bindings() = MakeBinding(
        "opaque_albedo_texture",
        2,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "opaque_normal_texture",
        3,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "opaque_roughness_texture",
        4,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "opaque_metallic_texture",
        5,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "opaque_ao_texture",
        6,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "skybox_env",
        7,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "TriangleBufferTransmissive",
        8,
        proto::ProgramBinding::STORAGE_BUFFER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "BvhBufferTransmissive",
        9,
        proto::ProgramBinding::STORAGE_BUFFER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "TriangleBufferOpaque",
        10,
        proto::ProgramBinding::STORAGE_BUFFER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "BvhBufferOpaque",
        11,
        proto::ProgramBinding::STORAGE_BUFFER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "UniformBlock",
        12,
        proto::ProgramBinding::UNIFORM_BUFFER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "skybox",
        13,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "transmissive_albedo_texture",
        14,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "transmissive_normal_texture",
        15,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "transmissive_roughness_texture",
        16,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "transmissive_metallic_texture",
        17,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "transmissive_ao_texture",
        18,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "transmissive_transmission_texture",
        19,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "transmissive_ior_texture",
        20,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "transmissive_thickness_texture",
        21,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "transmissive_attenuation_color_texture",
        22,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "transmissive_attenuation_distance_texture",
        23,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "opaque_specular_factor_texture",
        24,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "opaque_specular_color_texture",
        25,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "RaytraceInstanceBuffer",
        31,
        proto::ProgramBinding::STORAGE_BUFFER,
        {proto::ProgramStage::COMPUTE});
    AddSharedRaytracingUniforms(program);
    return MakeProgramInfo(
        std::move(program),
        {.vertex_shader = "raytrace.vert", .fragment_shader = "raytrace.frag"},
        {.vertex_shader = "raytrace.vert",
         .fragment_shader = "raytrace.frag",
         .compute_shader = "raytrace.comp",
         .raygen_shader = "raytrace.rgen",
         .miss_shader = "raytrace.rmiss",
         .closesthit_shader = "raytrace.rchit"});
}

ProgramInfo MakeRasterSceneProgram(const std::string& output_texture_name)
{
    proto::Program program;
    program.set_name("RasterSceneProgram");
    program.set_pipeline_name("raster_scene");
    program.add_output_texture_names(output_texture_name);
    program.mutable_input_scene_type()->set_value(proto::SceneType::SCENE);
    *program.add_bindings() = MakeBinding(
        "albedo_texture",
        0,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::FRAGMENT});
    program.add_input_texture_names("skybox");
    *program.add_bindings() = MakeBinding(
        "skybox",
        1,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::FRAGMENT});
    program.add_input_texture_names("skybox_env");
    *program.add_bindings() = MakeBinding(
        "skybox_env",
        2,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::FRAGMENT});
    *program.add_bindings() = MakeBinding(
        "normal_texture",
        3,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::FRAGMENT});
    *program.add_bindings() = MakeBinding(
        "roughness_texture",
        4,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::FRAGMENT});
    *program.add_bindings() = MakeBinding(
        "metallic_texture",
        5,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::FRAGMENT});
    *program.add_bindings() = MakeBinding(
        "ao_texture",
        6,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::FRAGMENT});
    *program.add_bindings() = MakeBinding(
        "transmission_texture",
        7,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::FRAGMENT});
    *program.add_bindings() = MakeBinding(
        "ior_texture",
        8,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::FRAGMENT});
    *program.add_bindings() = MakeBinding(
        "thickness_texture",
        9,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::FRAGMENT});
    *program.add_bindings() = MakeBinding(
        "attenuation_color_texture",
        10,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::FRAGMENT});
    *program.add_bindings() = MakeBinding(
        "specular_factor_texture",
        11,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::FRAGMENT});
    *program.add_bindings() = MakeBinding(
        "specular_color_texture",
        12,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::FRAGMENT});
    *program.add_bindings() = MakeBinding(
        "shadow_map",
        13,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::FRAGMENT});
    *program.add_bindings() = MakeBinding(
        "UniformBlock",
        14,
        proto::ProgramBinding::UNIFORM_BUFFER,
        {proto::ProgramStage::FRAGMENT});
    *program.add_uniforms() =
        MakeUniformEnum("projection", proto::Uniform::PROJECTION_MAT4);
    *program.add_uniforms() =
        MakeUniformEnum("view", proto::Uniform::VIEW_MAT4);
    *program.add_uniforms() =
        MakeUniformEnum("model", proto::Uniform::MODEL_MAT4);
    return MakeProgramInfo(
        std::move(program),
        {.vertex_shader = "raster_gltf.vert",
         .fragment_shader = "raster_gltf.frag"},
        {.vertex_shader = "raster_gltf.vert",
         .fragment_shader = "raster_gltf.frag"});
}

ProgramInfo MakeRaytracingPreprocessProgram(
    const std::string& pipeline_name,
    ShaderFiles opengl_files,
    ShaderFiles vulkan_files)
{
    proto::Program program;
    program.set_name("RayTracePreprocessProgram");
    program.set_pipeline_name(pipeline_name);
    program.mutable_input_scene_type()->set_value(proto::SceneType::SCENE);
    AddSharedRaytracingUniforms(program);
    return MakeProgramInfo(
        std::move(program), std::move(opengl_files), std::move(vulkan_files));
}

bool HasSkyboxRenderPass(const proto::Level& proto_level);

std::vector<ProgramInfo> BuildProgramsForPreset(
    SupportedScenePreset preset, const proto::Level& proto_level)
{
    switch (preset)
    {
    case SupportedScenePreset::Raster: {
        std::vector<ProgramInfo> programs = {};
        if (HasSkyboxRenderPass(proto_level))
        {
            programs.push_back(
                MakeCubemapProgram(proto_level.default_texture_name()));
        }
        programs.push_back(
            MakeRasterSceneProgram(proto_level.default_texture_name()));
        return programs;
    }
    case SupportedScenePreset::Cubemap:
        return {MakeCubemapProgram(proto_level.default_texture_name())};
    case SupportedScenePreset::Raytrace:
        return {
            MakeCubemapProgram(proto_level.default_texture_name()),
            MakeRaytraceProgram(proto_level.default_texture_name()),
            MakeRaytracingPreprocessProgram(
                "raytrace_preprocess",
                {.vertex_shader = "raytrace.vert",
                 .fragment_shader = "raytrace.frag"},
                {.vertex_shader = "raytrace.vert",
                 .fragment_shader = "raytrace.frag"})};
    case SupportedScenePreset::Unknown:
    default:
        throw std::runtime_error("Unsupported internal render preset.");
    }
}

bool HasSkyboxRenderPass(const proto::Level& proto_level)
{
    for (const auto& node : proto_level.scene_tree().node_meshes())
    {
        if (node.render_time_enum() == proto::NodeMesh::SKYBOX_RENDER_TIME)
        {
            return true;
        }
    }
    return false;
}

bool HasRasterSceneMesh(const proto::Level& proto_level)
{
    for (const auto& node : proto_level.scene_tree().node_meshes())
    {
        if (node.render_time_enum() != proto::NodeMesh::SCENE_RENDER_TIME)
        {
            continue;
        }
        if (node.has_clean_buffer())
        {
            continue;
        }
        if (ToLowerAscii(node.name()) == "raytracingrendering")
        {
            continue;
        }
        return true;
    }
    return false;
}

std::vector<RenderPassProgramInfo> BuildRenderPassProgramsForPreset(
    SupportedScenePreset preset, const proto::Level& proto_level)
{
    switch (preset)
    {
    case SupportedScenePreset::Raster: {
        std::vector<RenderPassProgramInfo> passes = {};
        if (HasSkyboxRenderPass(proto_level))
        {
            passes.push_back(
                {.render_time = proto::NodeMesh::SKYBOX_RENDER_TIME,
                 .program_name = "CubemapProgram"});
        }
        passes.push_back(
            {.render_time = proto::NodeMesh::SCENE_RENDER_TIME,
             .program_name = "RasterSceneProgram"});
        return passes;
    }
    case SupportedScenePreset::Cubemap:
        return {
            {.render_time = proto::NodeMesh::SKYBOX_RENDER_TIME,
             .program_name = "CubemapProgram"}};
    case SupportedScenePreset::Raytrace:
        return {
            {.render_time = proto::NodeMesh::SKYBOX_RENDER_TIME,
             .program_name = "CubemapProgram"},
            {.render_time = proto::NodeMesh::SCENE_RENDER_TIME,
             .program_name = "RayTraceProgram"},
            {.render_time = proto::NodeMesh::PRE_RENDER_TIME,
             .program_name = "RayTraceProgram",
             .preprocess_program_name = "RayTracePreprocessProgram"}};
    case SupportedScenePreset::Unknown:
    default:
        throw std::runtime_error("Unsupported internal render preset.");
    }
}

bool HasMeshFile(const proto::Level& proto_level, const std::string& file_name)
{
    const auto expected = ToLowerAscii(file_name);
    for (const auto& node : proto_level.scene_tree().node_meshes())
    {
        if (!node.has_file_name())
        {
            continue;
        }
        if (ToLowerAscii(node.file_name()).find(expected) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

void RemoveRaytracingResolveNodes(proto::Level& proto_level)
{
    if (!proto_level.has_scene_tree())
    {
        return;
    }
    auto* node_meshes = proto_level.mutable_scene_tree()->mutable_node_meshes();
    for (int i = node_meshes->size() - 1; i >= 0; --i)
    {
        if (ToLowerAscii(node_meshes->Get(i).name()) == "raytracingrendering")
        {
            node_meshes->DeleteSubrange(i, 1);
        }
    }
}

void DisableRaytracingAcceleration(proto::Level& proto_level)
{
    if (!proto_level.has_scene_tree())
    {
        return;
    }
    for (auto& node : *proto_level.mutable_scene_tree()->mutable_node_meshes())
    {
        node.set_acceleration_structure_enum(proto::NodeMesh::NO_ACCELERATION);
    }
}

SupportedScenePreset InferScenePreset(
    const proto::Level& proto_level,
    const std::filesystem::path& source_path,
    const LevelDataOptions& options)
{
    (void)source_path;
    switch (options.render_preset)
    {
    case RenderPreset::Raster:
        return HasRasterSceneMesh(proto_level) ? SupportedScenePreset::Raster
                                               : SupportedScenePreset::Cubemap;
    case RenderPreset::Cubemap:
        return SupportedScenePreset::Cubemap;
    case RenderPreset::Raytrace:
        return HasRasterSceneMesh(proto_level) ? SupportedScenePreset::Raytrace
                                               : SupportedScenePreset::Cubemap;
    case RenderPreset::Auto:
    default:
        break;
    }

    bool has_non_skybox_mesh = false;
    bool has_skybox_cube = false;
    for (const auto& node : proto_level.scene_tree().node_meshes())
    {
        if (node.render_time_enum() == proto::NodeMesh::SKYBOX_RENDER_TIME &&
            node.has_mesh_enum() && node.mesh_enum() == proto::NodeMesh::CUBE)
        {
            has_skybox_cube = true;
            continue;
        }
        if (!node.has_clean_buffer())
        {
            has_non_skybox_mesh = true;
        }
    }
    if (has_skybox_cube && !has_non_skybox_mesh)
    {
        return SupportedScenePreset::Cubemap;
    }
    if (has_non_skybox_mesh)
    {
        return SupportedScenePreset::Raytrace;
    }
    return SupportedScenePreset::Unknown;
}

} // namespace

LevelData BuildLevelData(
    glm::uvec2 /*size*/,
    const proto::Level& proto_level,
    const std::filesystem::path& asset_root,
    const std::filesystem::path& source_path,
    const LevelDataOptions& options)
{
    proto::Level prepared_level = proto_level;
    const auto preset = InferScenePreset(prepared_level, source_path, options);
    if (preset == SupportedScenePreset::Raster)
    {
        RemoveRaytracingResolveNodes(prepared_level);
        DisableRaytracingAcceleration(prepared_level);
    }

    LevelData data;
    data.proto = prepared_level;
    data.asset_root = asset_root;
    data.source_path = source_path;

    for (const auto& proto_texture : prepared_level.textures())
    {
        TextureInfo texture_info;
        texture_info.name = proto_texture.name();
        texture_info.element_size = proto_texture.pixel_element_size();
        texture_info.structure = proto_texture.pixel_structure();
        if (proto_texture.has_size())
        {
            texture_info.size = glm::uvec2(
                static_cast<std::uint32_t>(proto_texture.size().x()),
                static_cast<std::uint32_t>(proto_texture.size().y()));
        }
        data.textures.push_back(std::move(texture_info));
    }

    if (preset == SupportedScenePreset::Unknown)
    {
        if (!prepared_level.scene_tree().node_meshes().empty())
        {
            throw std::runtime_error(
                std::format(
                    "Unable to infer internal render preset for level '{}' "
                    "from '{}'.",
                    prepared_level.name(),
                    source_path.string()));
        }
        return data;
    }
    data.programs = BuildProgramsForPreset(preset, prepared_level);
    data.render_pass_programs =
        BuildRenderPassProgramsForPreset(preset, prepared_level);

    for (const auto& node : prepared_level.scene_tree().node_meshes())
    {
        if (node.mesh_enum() == frame::proto::NodeMesh::QUAD)
        {
            StaticMeshInfo mesh_info;
            mesh_info.name = node.name();
            mesh_info.positions = {
                -0.5f,
                -0.5f,
                0.0f,
                0.5f,
                -0.5f,
                0.0f,
                0.5f,
                0.5f,
                0.0f,
                -0.5f,
                0.5f,
                0.0f};
            mesh_info.uvs = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};
            mesh_info.normals = {
                0.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f};
            mesh_info.indices = {0, 1, 2, 2, 3, 0};
            data.meshes.push_back(std::move(mesh_info));
        }
        else if (node.mesh_enum() == frame::proto::NodeMesh::CUBE)
        {
            StaticMeshInfo mesh_info;
            mesh_info.name = node.name();
            mesh_info.positions = {
                // Front.
                -0.5f,
                -0.5f,
                -0.5f,
                0.5f,
                -0.5f,
                -0.5f,
                0.5f,
                0.5f,
                -0.5f,
                0.5f,
                0.5f,
                -0.5f,
                -0.5f,
                0.5f,
                -0.5f,
                -0.5f,
                -0.5f,
                -0.5f,
                // Back.
                -0.5f,
                -0.5f,
                0.5f,
                0.5f,
                -0.5f,
                0.5f,
                0.5f,
                0.5f,
                0.5f,
                0.5f,
                0.5f,
                0.5f,
                -0.5f,
                0.5f,
                0.5f,
                -0.5f,
                -0.5f,
                0.5f,
                // Left.
                -0.5f,
                0.5f,
                0.5f,
                -0.5f,
                0.5f,
                -0.5f,
                -0.5f,
                -0.5f,
                -0.5f,
                -0.5f,
                -0.5f,
                -0.5f,
                -0.5f,
                -0.5f,
                0.5f,
                -0.5f,
                0.5f,
                0.5f,
                // Right.
                0.5f,
                0.5f,
                0.5f,
                0.5f,
                0.5f,
                -0.5f,
                0.5f,
                -0.5f,
                -0.5f,
                0.5f,
                -0.5f,
                -0.5f,
                0.5f,
                -0.5f,
                0.5f,
                0.5f,
                0.5f,
                0.5f,
                // Bottom.
                -0.5f,
                -0.5f,
                -0.5f,
                0.5f,
                -0.5f,
                -0.5f,
                0.5f,
                -0.5f,
                0.5f,
                0.5f,
                -0.5f,
                0.5f,
                -0.5f,
                -0.5f,
                0.5f,
                -0.5f,
                -0.5f,
                -0.5f,
                // Top.
                -0.5f,
                0.5f,
                -0.5f,
                0.5f,
                0.5f,
                -0.5f,
                0.5f,
                0.5f,
                0.5f,
                0.5f,
                0.5f,
                0.5f,
                -0.5f,
                0.5f,
                0.5f,
                -0.5f,
                0.5f,
                -0.5f};
            mesh_info.uvs = {
                // Front.
                0.0f,
                0.0f,
                1.0f,
                0.0f,
                1.0f,
                1.0f,
                1.0f,
                1.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                // Back.
                0.0f,
                0.0f,
                1.0f,
                0.0f,
                1.0f,
                1.0f,
                1.0f,
                1.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                // Left.
                1.0f,
                0.0f,
                1.0f,
                1.0f,
                0.0f,
                1.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                0.0f,
                // Right.
                1.0f,
                0.0f,
                1.0f,
                1.0f,
                0.0f,
                1.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                0.0f,
                // Bottom.
                0.0f,
                1.0f,
                1.0f,
                1.0f,
                1.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                1.0f,
                // Top.
                0.0f,
                1.0f,
                1.0f,
                1.0f,
                1.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                1.0f};
            mesh_info.normals = {
                // Front.
                0.0f,
                0.0f,
                -1.0f,
                0.0f,
                0.0f,
                -1.0f,
                0.0f,
                0.0f,
                -1.0f,
                0.0f,
                0.0f,
                -1.0f,
                0.0f,
                0.0f,
                -1.0f,
                0.0f,
                0.0f,
                -1.0f,
                // Back.
                0.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                // Left.
                -1.0f,
                0.0f,
                0.0f,
                -1.0f,
                0.0f,
                0.0f,
                -1.0f,
                0.0f,
                0.0f,
                -1.0f,
                0.0f,
                0.0f,
                -1.0f,
                0.0f,
                0.0f,
                -1.0f,
                0.0f,
                0.0f,
                // Right.
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                // Bottom.
                0.0f,
                -1.0f,
                0.0f,
                0.0f,
                -1.0f,
                0.0f,
                0.0f,
                -1.0f,
                0.0f,
                0.0f,
                -1.0f,
                0.0f,
                0.0f,
                -1.0f,
                0.0f,
                0.0f,
                -1.0f,
                0.0f,
                // Top.
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                0.0f};
            mesh_info.indices.resize(36);
            std::iota(mesh_info.indices.begin(), mesh_info.indices.end(), 0);
            data.meshes.push_back(std::move(mesh_info));
        }
    }

    return data;
}

} // namespace frame::json
