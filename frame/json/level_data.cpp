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
    Cubemap,
    RaytracingSimple,
    Dragon,
    SkinnedMesh,
};

std::string ToLowerAscii(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
    return value;
}

proto::Uniform MakeUniformEnum(
    const std::string& name,
    proto::Uniform::UniformEnum uniform_enum)
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
    *program.add_uniforms() = MakeUniformEnum(
        "projection", proto::Uniform::PROJECTION_MAT4);
    *program.add_uniforms() = MakeUniformEnum(
        "view", proto::Uniform::VIEW_MAT4);
    *program.add_uniforms() = MakeUniformEnum(
        "model", proto::Uniform::MODEL_MAT4);
    *program.add_uniforms() = MakeUniformEnum(
        "time_s", proto::Uniform::FLOAT_TIME_S);
    return MakeProgramInfo(
        std::move(program),
        {.vertex_shader = "cubemap.vert", .fragment_shader = "cubemap.frag"},
        {.vertex_shader = "cubemap.vert", .fragment_shader = "cubemap.frag"});
}

void AddSharedRaytracingUniforms(proto::Program& program)
{
    *program.add_uniforms() = MakeUniformEnum(
        "projection", proto::Uniform::PROJECTION_MAT4);
    *program.add_uniforms() = MakeUniformEnum(
        "view", proto::Uniform::VIEW_MAT4);
    *program.add_uniforms() = MakeUniformEnum(
        "projection_inv", proto::Uniform::PROJECTION_INV_MAT4);
    *program.add_uniforms() = MakeUniformEnum(
        "view_inv", proto::Uniform::VIEW_INV_MAT4);
    *program.add_uniforms() = MakeUniformEnum(
        "model", proto::Uniform::MODEL_MAT4);
    *program.add_uniforms() = MakeUniformEnum(
        "model_inv", proto::Uniform::MODEL_INV_MAT4);
    *program.add_uniforms() = MakeUniformEnum(
        "camera_position", proto::Uniform::CAMERA_POSITION_VEC3);
    *program.add_uniforms() = MakeUniformEnum(
        "light_dir", proto::Uniform::LIGHT_POSITION_VEC3);
    *program.add_uniforms() = MakeUniformEnum(
        "light_color", proto::Uniform::LIGHT_COLOR_VEC3);
}

void AddSharedRaytracingInputs(
    proto::Program& program,
    const std::string& output_texture_name)
{
    program.add_output_texture_names(output_texture_name);
    program.add_input_texture_names("albedo_texture");
    program.add_input_texture_names("normal_texture");
    program.add_input_texture_names("roughness_texture");
    program.add_input_texture_names("metallic_texture");
    program.add_input_texture_names("ao_texture");
    program.add_input_texture_names("transmission_texture");
    program.add_input_texture_names("ior_texture");
    program.add_input_texture_names("thickness_texture");
    program.add_input_texture_names("attenuation_color_texture");
    program.add_input_texture_names("attenuation_distance_texture");
    program.add_input_texture_names("skybox");
    program.add_input_texture_names("skybox_env");
    program.mutable_input_scene_type()->set_value(proto::SceneType::QUAD);
    program.set_input_scene_root_name("mesh_holder");
}

void AddSimpleRaytracingSceneMaterialInputs(proto::Program& program)
{
    program.add_input_texture_names("opaque_albedo_texture");
    program.add_input_texture_names("opaque_normal_texture");
    program.add_input_texture_names("opaque_roughness_texture");
    program.add_input_texture_names("opaque_metallic_texture");
    program.add_input_texture_names("opaque_ao_texture");
    program.add_input_texture_names("transmissive_albedo_texture");
    program.add_input_texture_names("transmissive_normal_texture");
    program.add_input_texture_names("transmissive_roughness_texture");
    program.add_input_texture_names("transmissive_metallic_texture");
    program.add_input_texture_names("transmissive_ao_texture");
    program.add_input_texture_names("transmissive_transmission_texture");
    program.add_input_texture_names("transmissive_ior_texture");
    program.add_input_texture_names("transmissive_thickness_texture");
    program.add_input_texture_names("transmissive_attenuation_color_texture");
    program.add_input_texture_names("transmissive_attenuation_distance_texture");
}

ProgramInfo MakeRaytracingSimpleProgram(const std::string& output_texture_name)
{
    proto::Program program;
    program.set_name("RayTraceProgram");
    program.set_pipeline_name("raytracing_simple");
    AddSharedRaytracingInputs(program, output_texture_name);
    AddSimpleRaytracingSceneMaterialInputs(program);
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
        "albedo_texture",
        2,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "normal_texture",
        3,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "roughness_texture",
        4,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "metallic_texture",
        5,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "ao_texture",
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
        "TriangleBufferOpaque",
        9,
        proto::ProgramBinding::STORAGE_BUFFER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "UniformBlock",
        10,
        proto::ProgramBinding::UNIFORM_BUFFER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "skybox",
        11,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "transmission_texture",
        12,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "ior_texture",
        13,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "thickness_texture",
        14,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "attenuation_color_texture",
        15,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "attenuation_distance_texture",
        16,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "opaque_albedo_texture",
        17,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "opaque_normal_texture",
        18,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "opaque_roughness_texture",
        19,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "opaque_metallic_texture",
        20,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "opaque_ao_texture",
        21,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "transmissive_albedo_texture",
        22,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "transmissive_normal_texture",
        23,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "transmissive_roughness_texture",
        24,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "transmissive_metallic_texture",
        25,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "transmissive_ao_texture",
        26,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "transmissive_transmission_texture",
        27,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "transmissive_ior_texture",
        28,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "transmissive_thickness_texture",
        29,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "transmissive_attenuation_color_texture",
        30,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "transmissive_attenuation_distance_texture",
        31,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    AddSharedRaytracingUniforms(program);
    return MakeProgramInfo(
        std::move(program),
        {.vertex_shader = "raytracing_simple.vert",
         .fragment_shader = "raytracing_simple.frag"},
        {.vertex_shader = "raytracing_simple.vert",
         .fragment_shader = "raytracing_simple.frag",
         .compute_shader = "raytracing_dual.comp"});
}

ProgramInfo MakeDragonProgram(const std::string& output_texture_name)
{
    proto::Program program;
    program.set_name("RayTraceProgram");
    program.set_pipeline_name("dragon");
    AddSharedRaytracingInputs(program, output_texture_name);
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
        "albedo_texture",
        2,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "normal_texture",
        3,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "roughness_texture",
        4,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "metallic_texture",
        5,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "ao_texture",
        6,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "skybox_env",
        7,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "TriangleBuffer",
        8,
        proto::ProgramBinding::STORAGE_BUFFER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "BvhBuffer",
        9,
        proto::ProgramBinding::STORAGE_BUFFER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "UniformBlock",
        10,
        proto::ProgramBinding::UNIFORM_BUFFER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "skybox",
        11,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "transmission_texture",
        12,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "ior_texture",
        13,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "thickness_texture",
        14,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "attenuation_color_texture",
        15,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    *program.add_bindings() = MakeBinding(
        "attenuation_distance_texture",
        16,
        proto::ProgramBinding::COMBINED_IMAGE_SAMPLER,
        {proto::ProgramStage::COMPUTE});
    AddSharedRaytracingUniforms(program);
    return MakeProgramInfo(
        std::move(program),
        {.vertex_shader = "dragon.vert", .fragment_shader = "dragon.frag"},
        {.vertex_shader = "dragon.vert",
         .fragment_shader = "dragon.frag",
         .compute_shader = "dragon.comp"});
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
        std::move(program),
        std::move(opengl_files),
        std::move(vulkan_files));
}

std::vector<ProgramInfo> BuildProgramsForPreset(
    SupportedScenePreset preset,
    const proto::Level& proto_level)
{
    switch (preset)
    {
    case SupportedScenePreset::Cubemap:
        return {MakeCubemapProgram(proto_level.default_texture_name())};
    case SupportedScenePreset::RaytracingSimple:
        return {
            MakeCubemapProgram(proto_level.default_texture_name()),
            MakeRaytracingSimpleProgram(proto_level.default_texture_name()),
            MakeRaytracingPreprocessProgram(
                "raytracing_simple_preprocess",
                {.vertex_shader = "raytracing_simple.vert",
                 .fragment_shader = "raytracing_simple.frag"},
                {.vertex_shader = "raytracing_simple.vert",
                 .fragment_shader = "raytracing_simple.frag"})};
    case SupportedScenePreset::Dragon:
    case SupportedScenePreset::SkinnedMesh:
        return {
            MakeCubemapProgram(proto_level.default_texture_name()),
            MakeDragonProgram(proto_level.default_texture_name()),
            MakeRaytracingPreprocessProgram(
                "dragon_preprocess",
                {.vertex_shader = "dragon.vert", .fragment_shader = "dragon.frag"},
                {.vertex_shader = "dragon.vert", .fragment_shader = "dragon.frag"})};
    case SupportedScenePreset::Unknown:
    default:
        throw std::runtime_error("Unsupported internal render preset.");
    }
}

std::vector<RenderPassProgramInfo> BuildRenderPassProgramsForPreset(
    SupportedScenePreset preset)
{
    switch (preset)
    {
    case SupportedScenePreset::Cubemap:
        return {{
            .render_time = proto::NodeMesh::SKYBOX_RENDER_TIME,
            .program_name = "CubemapProgram"}};
    case SupportedScenePreset::RaytracingSimple:
    case SupportedScenePreset::Dragon:
    case SupportedScenePreset::SkinnedMesh:
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

bool HasMeshFile(
    const proto::Level& proto_level, const std::string& file_name)
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

SupportedScenePreset InferScenePreset(
    const proto::Level& proto_level,
    const std::filesystem::path& source_path)
{
    const auto stem = ToLowerAscii(source_path.stem().string());
    if (stem == "cubemap")
    {
        return SupportedScenePreset::Cubemap;
    }
    if (stem == "raytracing")
    {
        return SupportedScenePreset::RaytracingSimple;
    }
    if (stem == "dragon")
    {
        return SupportedScenePreset::Dragon;
    }
    if (stem == "skinned_mesh")
    {
        return SupportedScenePreset::SkinnedMesh;
    }

    for (const auto& node : proto_level.scene_tree().node_meshes())
    {
        if (node.play_animation())
        {
            return SupportedScenePreset::SkinnedMesh;
        }
    }
    if (HasMeshFile(proto_level, "fox.glb"))
    {
        return SupportedScenePreset::SkinnedMesh;
    }
    if (HasMeshFile(proto_level, "dragon.glb"))
    {
        return SupportedScenePreset::Dragon;
    }
    if (HasMeshFile(proto_level, "ico.glb") || HasMeshFile(proto_level, "plate.glb"))
    {
        return SupportedScenePreset::RaytracingSimple;
    }

    bool has_non_skybox_mesh = false;
    bool has_skybox_cube = false;
    for (const auto& node : proto_level.scene_tree().node_meshes())
    {
        if (node.render_time_enum() == proto::NodeMesh::SKYBOX_RENDER_TIME &&
            node.has_mesh_enum() &&
            node.mesh_enum() == proto::NodeMesh::CUBE)
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
    return SupportedScenePreset::Unknown;
}

} // namespace

LevelData BuildLevelData(
    glm::uvec2 /*size*/,
    const proto::Level& proto_level,
    const std::filesystem::path& asset_root,
    const std::filesystem::path& source_path)
{
    LevelData data;
    data.proto = proto_level;
    data.asset_root = asset_root;
    data.source_path = source_path;

    for (const auto& proto_texture : proto_level.textures())
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

    const auto preset = InferScenePreset(proto_level, source_path);
    if (preset == SupportedScenePreset::Unknown)
    {
        throw std::runtime_error(std::format(
            "Unable to infer internal render preset for level '{}' from '{}'.",
            proto_level.name(),
            source_path.string()));
    }
    data.programs = BuildProgramsForPreset(preset, proto_level);
    data.render_pass_programs = BuildRenderPassProgramsForPreset(preset);

    for (const auto& node : proto_level.scene_tree().node_meshes())
    {
        if (node.mesh_enum() == frame::proto::NodeMesh::QUAD)
        {
            StaticMeshInfo mesh_info;
            mesh_info.name = node.name();
            mesh_info.positions = {
                -0.5f, -0.5f, 0.0f,
                0.5f, -0.5f, 0.0f,
                0.5f, 0.5f, 0.0f,
                -0.5f, 0.5f, 0.0f};
            mesh_info.uvs = {
                0.0f, 0.0f,
                1.0f, 0.0f,
                1.0f, 1.0f,
                0.0f, 1.0f};
            mesh_info.indices = {0, 1, 2, 2, 3, 0};
            data.meshes.push_back(std::move(mesh_info));
        }
        else if (node.mesh_enum() == frame::proto::NodeMesh::CUBE)
        {
            StaticMeshInfo mesh_info;
            mesh_info.name = node.name();
            mesh_info.positions = {
                // Front.
                -0.5f, -0.5f, -0.5f,
                0.5f, -0.5f, -0.5f,
                0.5f, 0.5f, -0.5f,
                0.5f, 0.5f, -0.5f,
                -0.5f, 0.5f, -0.5f,
                -0.5f, -0.5f, -0.5f,
                // Back.
                -0.5f, -0.5f, 0.5f,
                0.5f, -0.5f, 0.5f,
                0.5f, 0.5f, 0.5f,
                0.5f, 0.5f, 0.5f,
                -0.5f, 0.5f, 0.5f,
                -0.5f, -0.5f, 0.5f,
                // Left.
                -0.5f, 0.5f, 0.5f,
                -0.5f, 0.5f, -0.5f,
                -0.5f, -0.5f, -0.5f,
                -0.5f, -0.5f, -0.5f,
                -0.5f, -0.5f, 0.5f,
                -0.5f, 0.5f, 0.5f,
                // Right.
                0.5f, 0.5f, 0.5f,
                0.5f, 0.5f, -0.5f,
                0.5f, -0.5f, -0.5f,
                0.5f, -0.5f, -0.5f,
                0.5f, -0.5f, 0.5f,
                0.5f, 0.5f, 0.5f,
                // Bottom.
                -0.5f, -0.5f, -0.5f,
                0.5f, -0.5f, -0.5f,
                0.5f, -0.5f, 0.5f,
                0.5f, -0.5f, 0.5f,
                -0.5f, -0.5f, 0.5f,
                -0.5f, -0.5f, -0.5f,
                // Top.
                -0.5f, 0.5f, -0.5f,
                0.5f, 0.5f, -0.5f,
                0.5f, 0.5f, 0.5f,
                0.5f, 0.5f, 0.5f,
                -0.5f, 0.5f, 0.5f,
                -0.5f, 0.5f, -0.5f};
            mesh_info.uvs = {
                // Front.
                0.0f, 0.0f,
                1.0f, 0.0f,
                1.0f, 1.0f,
                1.0f, 1.0f,
                0.0f, 1.0f,
                0.0f, 0.0f,
                // Back.
                0.0f, 0.0f,
                1.0f, 0.0f,
                1.0f, 1.0f,
                1.0f, 1.0f,
                0.0f, 1.0f,
                0.0f, 0.0f,
                // Left.
                1.0f, 0.0f,
                1.0f, 1.0f,
                0.0f, 1.0f,
                0.0f, 1.0f,
                0.0f, 0.0f,
                1.0f, 0.0f,
                // Right.
                1.0f, 0.0f,
                1.0f, 1.0f,
                0.0f, 1.0f,
                0.0f, 1.0f,
                0.0f, 0.0f,
                1.0f, 0.0f,
                // Bottom.
                0.0f, 1.0f,
                1.0f, 1.0f,
                1.0f, 0.0f,
                1.0f, 0.0f,
                0.0f, 0.0f,
                0.0f, 1.0f,
                // Top.
                0.0f, 1.0f,
                1.0f, 1.0f,
                1.0f, 0.0f,
                1.0f, 0.0f,
                0.0f, 0.0f,
                0.0f, 1.0f};
            mesh_info.indices.resize(36);
            std::iota(mesh_info.indices.begin(), mesh_info.indices.end(), 0);
            data.meshes.push_back(std::move(mesh_info));
        }
    }

    return data;
}

} // namespace frame::json
