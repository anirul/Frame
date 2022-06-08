#include "frame/json/parse_program.h"

#include <fstream>

#include "frame/file/file_system.h"
#include "frame/open_gl/program.h"

namespace frame::proto {

std::optional<std::unique_ptr<frame::ProgramInterface>> ParseProgramOpenGL(
    const Program& proto_program, const LevelInterface* level) {
    Logger& logger          = Logger::GetInstance();
    std::string shader_path = "asset/shader/open_gl/";
    std::string shader_name = shader_path + proto_program.shader();
    std::string shader_vert = file::FindFile(shader_name + ".vert");
    logger->info("Openning vertex shader: [{}].", shader_vert);
    std::ifstream ifs_vertex(shader_vert);
    if (!ifs_vertex.is_open()) {
        throw std::runtime_error(fmt::format("Couldn't open file {}.vert", shader_name));
    }
    std::string shader_frag = file::FindFile(shader_name + ".frag");
    logger->info("Openning fragment shader: [{}].", shader_frag);
    std::ifstream ifs_pixel(shader_frag);
    if (!ifs_pixel.is_open()) {
        throw std::runtime_error(fmt::format("Couldn't open file {}.frag", shader_name));
    }
    auto maybe_program = opengl::CreateProgram(ifs_vertex, ifs_pixel);
    if (!maybe_program) return std::nullopt;
    auto program = std::move(maybe_program.value());
    for (const auto& texture_name : proto_program.input_texture_names()) {
        auto maybe_texture_id = level->GetIdFromName(texture_name);
        if (!maybe_texture_id) return std::nullopt;
        EntityId texture_id = maybe_texture_id.value();
        // Check this is a texture.
        program->AddInputTextureId(texture_id);
    }
    for (const auto& texture_name : proto_program.output_texture_names()) {
        auto maybe_texture_id = level->GetIdFromName(texture_name);
        if (!maybe_texture_id) return std::nullopt;
        EntityId texture_id = maybe_texture_id.value();
        // Check this is a texture.
        program->AddOutputTextureId(texture_id);
    }
    program->SetSceneRoot(0);
    switch (proto_program.input_scene_type().value()) {
        case SceneType::QUAD: {
            auto maybe_quad_id = level->GetDefaultStaticMeshQuadId();
            if (!maybe_quad_id) return std::nullopt;
            EntityId quad_id = maybe_quad_id.value();
            program->SetSceneRoot(quad_id);
            break;
        }
        case SceneType::CUBE: {
            auto maybe_cube_id = level->GetDefaultStaticMeshCubeId();
            if (!maybe_cube_id) return std::nullopt;
            EntityId cube_id = maybe_cube_id.value();
            program->SetSceneRoot(cube_id);
            break;
        }
        case SceneType::SCENE: {
            program->SetTemporarySceneRoot(proto_program.input_scene_root_name());
            break;
        }
        case SceneType::NONE:
        default:
            throw std::runtime_error(
                fmt::format("No way {}?", proto_program.input_scene_type().value()));
    }
    for (const auto& parameter : proto_program.parameters()) {
        program->Uniform(parameter.name(), parameter.uniform_enum());
    }
    return program;
}

}  // End namespace frame::proto.
