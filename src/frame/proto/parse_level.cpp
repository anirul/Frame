#include "frame/proto/parse_level.h"

#include "frame/level.h"
#include "frame/open_gl/material.h"
#include "frame/open_gl/static_mesh.h"
#include "frame/open_gl/texture.h"
#include "frame/program_interface.h"
#include "frame/proto/parse_material.h"
#include "frame/proto/parse_program.h"
#include "frame/proto/parse_scene_tree.h"
#include "frame/proto/parse_texture.h"

namespace frame::proto {

class LevelProto : public frame::Level {
   public:
    LevelProto(const std::pair<std::int32_t, std::int32_t> size, const proto::Level& proto_level) {
        name_                 = proto_level.name();
        default_texture_name_ = proto_level.default_texture_name();
        if (default_texture_name_.empty())
            throw std::runtime_error("should have a default texture.");

        // Include the default cube and quad.
        auto maybe_cube_id = opengl::CreateCubeStaticMesh(this);
        if (!maybe_cube_id) throw std::runtime_error("Could not create static cube mesh.");
        cube_id_           = maybe_cube_id.value();
        auto maybe_quad_id = opengl::CreateQuadStaticMesh(this);
        if (!maybe_quad_id) throw std::runtime_error("Could not create static quad mesh.");
        quad_id_ = maybe_quad_id.value();

        // Load textures from proto.
        for (const auto& proto_texture : proto_level.textures()) {
            std::unique_ptr<TextureInterface> texture = nullptr;
            if (proto_texture.cubemap()) {
                if (proto_texture.file_name().empty() && proto_texture.file_names().empty()) {
                    texture = ParseCubeMapTexture(proto_texture, size);
                } else {
                    texture = ParseCubeMapTextureFile(proto_texture);
                }
            } else {
                if (proto_texture.file_name().empty()) {
                    texture = ParseTexture(proto_texture, size);
                } else {
                    texture = ParseTextureFile(proto_texture);
                }
            }
            if (!texture) {
                throw std::runtime_error(
                    fmt::format("Could not load texture: {}", proto_texture.file_name()));
            }
            texture->SetName(proto_texture.name());
            if (!AddTexture(std::move(texture))) {
                throw std::runtime_error(
                    fmt::format("Coudn't save texture {} to level.", proto_texture.name()));
            }
        }

        // Check the default texture is in.
        if (name_id_map_.find(default_texture_name_) == name_id_map_.end()) {
            throw std::runtime_error("no default texture is loaded: " + default_texture_name_);
        }

        // Load programs from proto.
        for (const auto& proto_program : proto_level.programs()) {
            auto maybe_program = ParseProgramOpenGL(proto_program, this);
            if (!maybe_program) {
                throw std::runtime_error(fmt::format("invalid program: {}", proto_program.name()));
            }
            auto program = std::move(maybe_program.value());
            program->SetName(proto_program.name());
            if (!AddProgram(std::move(program))) {
                throw std::runtime_error(
                    fmt::format("Couldn't save program {} to level.", proto_program.name()));
            }
        }

        // Load material from proto.
        for (const auto& proto_material : proto_level.materials()) {
            auto maybe_material = ParseMaterialOpenGL(proto_material, this);
            if (!maybe_material) {
                throw std::runtime_error(
                    fmt::format("invalid material : {}", proto_material.name()));
            }
            auto material = std::move(maybe_material.value());
            if (!AddMaterial(std::move(material))) {
                throw std::runtime_error(
                    fmt::format("Couldn't save material {} to level.", proto_material.name()));
            }
        }

        // Load scenes from proto.
        if (!ParseSceneTreeFile(proto_level.scene_tree(), this))
            throw std::runtime_error("Could not parse proto scene file.");
        default_camera_name_ = proto_level.scene_tree().default_camera_name();
    }
};

std::optional<std::unique_ptr<LevelInterface>> ParseLevelOpenGL(
    const std::pair<std::int32_t, std::int32_t> size, const proto::Level& proto_level) {
    auto ptr = std::make_unique<LevelProto>(size, proto_level);
    if (ptr) return ptr;
    return std::nullopt;
}

}  // End namespace frame::proto.
