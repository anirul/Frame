#include "frame/json/parse_program.h"

#include <filesystem>
#include <fstream>

#include "frame/file/file_system.h"
#include "frame/json/parse_uniform.h"
#include "frame/opengl/file/load_program.h"
#include "frame/opengl/program.h"

namespace frame::proto {

std::unique_ptr<frame::ProgramInterface> ParseProgramOpenGL(
    const Program& proto_program, LevelInterface& level) {
  Logger& logger = Logger::GetInstance();
  // Create the program.
  auto program = opengl::file::LoadProgram(proto_program.shader());
  if (!program) return nullptr;
  for (const auto& texture_name : proto_program.input_texture_names()) {
    auto maybe_texture_id = level.GetIdFromName(texture_name);
    if (!maybe_texture_id) return nullptr;
    EntityId texture_id = maybe_texture_id;
    // Check this is a texture.
    program->AddInputTextureId(texture_id);
  }
  for (const auto& texture_name : proto_program.output_texture_names()) {
    auto maybe_texture_id = level.GetIdFromName(texture_name);
    if (!maybe_texture_id) return nullptr;
    EntityId texture_id = maybe_texture_id;
    // Check this is a texture.
    program->AddOutputTextureId(texture_id);
  }
  program->SetSceneRoot(0);
  switch (proto_program.input_scene_type().value()) {
    case SceneType::QUAD: {
      auto maybe_quad_id = level.GetDefaultStaticMeshQuadId();
      if (!maybe_quad_id) return nullptr;
      EntityId quad_id = maybe_quad_id;
      program->SetSceneRoot(quad_id);
      break;
    }
    case SceneType::CUBE: {
      auto maybe_cube_id = level.GetDefaultStaticMeshCubeId();
      if (!maybe_cube_id) return nullptr;
      EntityId cube_id = maybe_cube_id;
      program->SetSceneRoot(cube_id);
      break;
    }
    case SceneType::SCENE: {
      program->SetTemporarySceneRoot(proto_program.input_scene_root_name());
      break;
    }
    case SceneType::NONE:
    default:
      throw std::runtime_error(fmt::format(
          "No way {}?",
          static_cast<int>(proto_program.input_scene_type().value())));
  }
  for (const auto& parameter : proto_program.parameters()) {
    switch (parameter.value_oneof_case()) {
      case Uniform::kUniformEnum:
        break;
      case Uniform::kUniformInt:
        program->Uniform(parameter.name(), parameter.uniform_int());
        break;
      case Uniform::kUniformFloat:
        program->Uniform(parameter.name(), parameter.uniform_float());
        break;
      case Uniform::kUniformVec2:
        program->Uniform(parameter.name(),
                         ParseUniform(parameter.uniform_vec2()));
        break;
      case Uniform::kUniformVec3:
        program->Uniform(parameter.name(),
                         ParseUniform(parameter.uniform_vec3()));
        break;
      case Uniform::kUniformVec4:
        program->Uniform(parameter.name(),
                         ParseUniform(parameter.uniform_vec4()));
        break;
      case Uniform::kUniformMat4:
        program->Uniform(parameter.name(),
                         ParseUniform(parameter.uniform_mat4()));
        break;
      case Uniform::kUniformFloatPlugin:
        break;
      case Uniform::kUniformIntPlugin:
        break;
      default:
        throw std::runtime_error(
            fmt::format("No handle for parameter {}#?",
                        static_cast<int>(parameter.value_oneof_case())));
    }
  }
  return program;
}

}  // End namespace frame::proto.
