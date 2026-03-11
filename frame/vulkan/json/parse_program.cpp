#include "frame/vulkan/json/parse_program.h"

#include <array>
#include <format>
#include <stdexcept>

#include "frame/json/parse_pixel.h"
#include "frame/json/program_catalog.h"
#include "frame/json/parse_uniform.h"
#include "frame/uniform.h"
#include "frame/vulkan/json/parse_texture.h"
#include "frame/vulkan/program.h"

namespace frame::vulkan::json
{

namespace
{

struct GeneratedTextureSpec
{
    std::array<float, 4> color = {0.0f, 0.0f, 0.0f, 1.0f};
    frame::proto::PixelElementSize::Enum element =
        frame::proto::PixelElementSize::BYTE;
};

std::optional<GeneratedTextureSpec> GetRaytracingTextureSpec(
    const std::string& texture_name)
{
    if (texture_name == "transmission_texture")
    {
        return GeneratedTextureSpec{};
    }
    if (texture_name == "ior_texture")
    {
        return GeneratedTextureSpec{
            .color = {1.5f, 1.5f, 1.5f, 1.0f},
            .element = frame::proto::PixelElementSize::FLOAT};
    }
    if (texture_name == "thickness_texture")
    {
        return GeneratedTextureSpec{
            .color = {0.0f, 0.0f, 0.0f, 1.0f},
            .element = frame::proto::PixelElementSize::FLOAT};
    }
    if (texture_name == "attenuation_color_texture")
    {
        return GeneratedTextureSpec{
            .color = {1.0f, 1.0f, 1.0f, 1.0f},
            .element = frame::proto::PixelElementSize::BYTE};
    }
    if (texture_name == "attenuation_distance_texture")
    {
        return GeneratedTextureSpec{
            .color = {1000000.0f, 1000000.0f, 1000000.0f, 1.0f},
            .element = frame::proto::PixelElementSize::FLOAT};
    }
    return std::nullopt;
}

frame::EntityId EnsureRaytracingDefaultTexture(
    const frame::proto::Program& proto_program,
    const std::string& texture_name,
    frame::LevelInterface& level)
{
    const auto key = frame::json::ResolveProgramKey(proto_program);
    if (!frame::json::IsRaytracingProgramKey(key))
    {
        return frame::NullId;
    }
    const auto spec = GetRaytracingTextureSpec(texture_name);
    if (!spec)
    {
        return frame::NullId;
    }

    frame::proto::Texture proto_texture;
    proto_texture.set_name(texture_name);
    proto_texture.mutable_size()->set_x(1);
    proto_texture.mutable_size()->set_y(1);
    proto_texture.mutable_pixel_structure()->CopyFrom(
        frame::json::PixelStructure_RGB_ALPHA());
    proto_texture.mutable_pixel_element_size()->set_value(spec->element);
    if (spec->element == frame::proto::PixelElementSize::FLOAT)
    {
        proto_texture.set_pixels(
            reinterpret_cast<const char*>(spec->color.data()),
            static_cast<int>(spec->color.size() * sizeof(float)));
    }
    else
    {
        std::array<std::uint8_t, 4> pixels = {
            static_cast<std::uint8_t>(spec->color[0] * 255.0f),
            static_cast<std::uint8_t>(spec->color[1] * 255.0f),
            static_cast<std::uint8_t>(spec->color[2] * 255.0f),
            static_cast<std::uint8_t>(spec->color[3] * 255.0f)};
        proto_texture.set_pixels(
            reinterpret_cast<const char*>(pixels.data()),
            static_cast<int>(pixels.size()));
    }
    auto texture = frame::vulkan::json::ParseTexture(proto_texture, {1u, 1u});
    texture->SetName(texture_name);
    texture->SetSerializeEnable(false);
    return level.AddTexture(std::move(texture));
}

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
        auto quad_id = level.GetDefaultMeshQuadId();
        if (!quad_id)
        {
            throw std::runtime_error("Default quad static mesh not available.");
        }
        program.SetSceneRoot(quad_id);
        break;
    }
    case frame::proto::SceneType::CUBE: {
        auto cube_id = level.GetDefaultMeshCubeId();
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
            texture_id = EnsureRaytracingDefaultTexture(
                proto_program, texture_name, level);
        }
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
