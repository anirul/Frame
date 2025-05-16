#include "frame/json/parse_program.h"

#include <filesystem>
#include <fstream>

#include "frame/file/file_system.h"
#include "frame/json/parse_uniform.h"
#include "frame/opengl/file/load_program.h"
#include "frame/opengl/program.h"
#include "frame/uniform.h"

namespace frame::json
{

std::unique_ptr<frame::ProgramInterface> ParseProgramOpenGL(
    const proto::Program& proto_program, LevelInterface& level)
{
    Logger& logger = Logger::GetInstance();
    // Create the program.
    auto program = opengl::file::LoadProgram(proto_program);
    if (!program)
    {
        return nullptr;
    }
    for (const auto& texture_name : proto_program.input_texture_names())
    {
        EntityId texture_id = level.GetIdFromName(texture_name);
        if (!texture_id)
        {
            return nullptr;
        }
        program->AddInputTextureId(texture_id);
    }
    for (const auto& texture_name : proto_program.output_texture_names())
    {
        EntityId texture_id = level.GetIdFromName(texture_name);
        if (!texture_id)
        {
            return nullptr;
        }
        program->AddOutputTextureId(texture_id);
    }
    program->SetSceneRoot(0);
    switch (proto_program.input_scene_type().value())
    {
    case proto::SceneType::QUAD: {
        EntityId quad_id = level.GetDefaultStaticMeshQuadId();
        if (!quad_id)
        {
            return nullptr;
        }
        program->SetSceneRoot(quad_id);
        break;
    }
    case proto::SceneType::CUBE: {
        auto maybe_cube_id = level.GetDefaultStaticMeshCubeId();
        if (!maybe_cube_id)
            return nullptr;
        EntityId cube_id = maybe_cube_id;
        program->SetSceneRoot(cube_id);
        break;
    }
    case proto::SceneType::SCENE: {
        program->SetTemporarySceneRoot(proto_program.input_scene_root_name());
        break;
    }
    case proto::SceneType::NONE:
    default:
        throw std::runtime_error(
            fmt::format(
                "No way {}?",
                static_cast<int>(proto_program.input_scene_type().value())));
    }
    program->Use();
    for (const auto& parameter : proto_program.parameters())
    {
        switch (parameter.value_oneof_case())
        {
        case proto::Uniform::kUniformEnum: {
            std::unique_ptr<UniformInterface> uniform_interface =
                std::make_unique<frame::Uniform>(
                    parameter.name(), parameter.uniform_enum());
            program->AddUniform(std::move(uniform_interface));
            break;
        }
        case proto::Uniform::kUniformInt: {
            std::unique_ptr<UniformInterface> uniform_interface =
                std::make_unique<frame::Uniform>(
                    parameter.name(), parameter.uniform_int());
            program->AddUniform(std::move(uniform_interface));
            break;
        }
        case proto::Uniform::kUniformInts: {
            proto::UniformInts uniform_ints = parameter.uniform_ints();
            glm::uvec2 glm_size =
                glm::uvec2(uniform_ints.size().x(), uniform_ints.size().y());
            std::vector<int> int_list;
            int_list.assign(
                uniform_ints.values().begin(), uniform_ints.values().end());
            std::unique_ptr<UniformInterface> uniform_interface =
                std::make_unique<frame::Uniform>(
                    parameter.name(), glm_size, int_list);
            break;
        }
        case proto::Uniform::kUniformFloat: {
            std::unique_ptr<UniformInterface> uniform_interface =
                std::make_unique<frame::Uniform>(
                    parameter.name(), parameter.uniform_float());
            program->AddUniform(std::move(uniform_interface));
            break;
        }
        case proto::Uniform::kUniformFloats: {
            proto::UniformFloats uniform_floats = parameter.uniform_floats();
            glm::uvec2 glm_size = glm::uvec2(
                uniform_floats.size().x(), uniform_floats.size().y());
            std::vector<float> float_list;
            float_list.assign(
                uniform_floats.values().begin(), uniform_floats.values().end());
            std::unique_ptr<UniformInterface> uniform_interface =
                std::make_unique<frame::Uniform>(
                    parameter.name(), glm_size, float_list);
            break;
        }
        case proto::Uniform::kUniformVec2: {
            std::unique_ptr<UniformInterface> uniform_interface =
                std::make_unique<frame::Uniform>(
                    parameter.name(), ParseUniform(parameter.uniform_vec2()));
            program->AddUniform(std::move(uniform_interface));
            break;
        }
        case proto::Uniform::kUniformVec3: {
            std::unique_ptr<UniformInterface> uniform_interface =
                std::make_unique<frame::Uniform>(
                    parameter.name(), ParseUniform(parameter.uniform_vec3()));
            program->AddUniform(std::move(uniform_interface));
            break;
        }
        case proto::Uniform::kUniformVec4: {
            std::unique_ptr<UniformInterface> uniform_interface =
                std::make_unique<frame::Uniform>(
                    parameter.name(), ParseUniform(parameter.uniform_vec4()));
            program->AddUniform(std::move(uniform_interface));
            break;
        }
        case proto::Uniform::kUniformMat4: {
            std::unique_ptr<UniformInterface> uniform_interface =
                std::make_unique<frame::Uniform>(
                    parameter.name(), ParseUniform(parameter.uniform_mat4()));
            program->AddUniform(std::move(uniform_interface));
            break;
        }
        case proto::Uniform::kUniformFloatPlugin:
            break;
        case proto::Uniform::kUniformIntPlugin:
            break;
        default:
            throw std::runtime_error(
                std::format(
                    "No handle for parameter {}#?",
                    static_cast<int>(parameter.value_oneof_case())));
        }
    }
    program->UnUse();
    return program;
}

} // End namespace frame::json.
