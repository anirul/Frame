#include "frame/vulkan/json/parse_program.h"

#include <format>
#include <stdexcept>

#include "frame/json/parse_uniform.h"
#include "frame/uniform.h"
#include "frame/vulkan/program.h"

namespace frame::vulkan::json
{

namespace
{

std::unique_ptr<frame::UniformInterface> MakeUniform(
    const frame::proto::Uniform& proto_uniform)
{
    switch (proto_uniform.value_oneof_case())
    {
    case frame::proto::Uniform::kUniformEnum:
        return std::make_unique<frame::Uniform>(
            proto_uniform.name(), proto_uniform.uniform_enum());
    case frame::proto::Uniform::kUniformInt:
        return std::make_unique<frame::Uniform>(
            proto_uniform.name(), proto_uniform.uniform_int());
    case frame::proto::Uniform::kUniformInts: {
        glm::uvec2 size(
            proto_uniform.uniform_ints().size().x(),
            proto_uniform.uniform_ints().size().y());
        std::vector<int> values(
            proto_uniform.uniform_ints().values().begin(),
            proto_uniform.uniform_ints().values().end());
        return std::make_unique<frame::Uniform>(
            proto_uniform.name(), size, values);
    }
    case frame::proto::Uniform::kUniformFloat:
        return std::make_unique<frame::Uniform>(
            proto_uniform.name(), proto_uniform.uniform_float());
    case frame::proto::Uniform::kUniformFloats: {
        glm::uvec2 size(
            proto_uniform.uniform_floats().size().x(),
            proto_uniform.uniform_floats().size().y());
        std::vector<float> values(
            proto_uniform.uniform_floats().values().begin(),
            proto_uniform.uniform_floats().values().end());
        return std::make_unique<frame::Uniform>(
            proto_uniform.name(), size, values);
    }
    case frame::proto::Uniform::kUniformVec2:
        return std::make_unique<frame::Uniform>(
            proto_uniform.name(), frame::json::ParseUniform(
                                     proto_uniform.uniform_vec2()));
    case frame::proto::Uniform::kUniformVec3:
        return std::make_unique<frame::Uniform>(
            proto_uniform.name(), frame::json::ParseUniform(
                                     proto_uniform.uniform_vec3()));
    case frame::proto::Uniform::kUniformVec4:
        return std::make_unique<frame::Uniform>(
            proto_uniform.name(), frame::json::ParseUniform(
                                     proto_uniform.uniform_vec4()));
    case frame::proto::Uniform::kUniformMat4:
        return std::make_unique<frame::Uniform>(
            proto_uniform.name(), frame::json::ParseUniform(
                                     proto_uniform.uniform_mat4()));
    case frame::proto::Uniform::kUniformFloatPlugin:
    case frame::proto::Uniform::kUniformIntPlugin:
        // Plugins are not supported yet in Vulkan path.
        return nullptr;
    case frame::proto::Uniform::VALUE_ONEOF_NOT_SET:
    default:
        throw std::runtime_error(std::format(
            "Unsupported uniform type for {}.", proto_uniform.name()));
    }
}

void ConfigureSceneRoot(
    const frame::proto::Program& proto_program,
    frame::LevelInterface& level,
    frame::ProgramInterface& program)
{
    const auto scene_type = proto_program.input_scene_type().value();
    switch (scene_type)
    {
    case frame::proto::SceneType::QUAD: {
        auto quad_id = level.GetDefaultStaticMeshQuadId();
        if (!quad_id)
        {
            throw std::runtime_error("Default quad static mesh not available.");
        }
        program.SetSceneRoot(quad_id);
        break;
    }
    case frame::proto::SceneType::CUBE: {
        auto cube_id = level.GetDefaultStaticMeshCubeId();
        if (!cube_id)
        {
            throw std::runtime_error("Default cube static mesh not available.");
        }
        program.SetSceneRoot(cube_id);
        break;
    }
    case frame::proto::SceneType::SCENE:
        break;
    case frame::proto::SceneType::NONE:
    default:
        throw std::runtime_error("Unsupported scene type for program.");
    }

    if (!proto_program.input_scene_root_name().empty() &&
        proto_program.input_scene_root_name() != "root")
    {
        program.SetTemporarySceneRoot(proto_program.input_scene_root_name());
    }
}

} // namespace

std::unique_ptr<frame::ProgramInterface> ParseProgram(
    const frame::proto::Program& proto_program,
    frame::LevelInterface& level)
{
    auto program = std::make_unique<frame::vulkan::Program>(
        proto_program.name());
    program->SetSerializeEnable(true);

    auto proto_copy = proto_program;
    program->FromProto(std::move(proto_copy));

    for (const auto& texture_name : proto_program.input_texture_names())
    {
        auto texture_id = level.GetIdFromName(texture_name);
        if (texture_id == frame::NullId)
        {
            throw std::runtime_error(std::format(
                "Input texture {} not found for program {}.",
                texture_name,
                proto_program.name()));
        }
        program->AddInputTextureId(texture_id);
    }

    for (const auto& texture_name : proto_program.output_texture_names())
    {
        auto texture_id = level.GetIdFromName(texture_name);
        if (texture_id == frame::NullId)
        {
            throw std::runtime_error(std::format(
                "Output texture {} not found for program {}.",
                texture_name,
                proto_program.name()));
        }
        program->AddOutputTextureId(texture_id);
    }

    ConfigureSceneRoot(proto_program, level, *program);

    for (const auto& uniform_proto : proto_program.uniforms())
    {
        auto uniform = MakeUniform(uniform_proto);
        if (uniform)
        {
            program->AddUniform(std::move(uniform));
        }
    }

    return program;
}

} // namespace frame::vulkan::json
