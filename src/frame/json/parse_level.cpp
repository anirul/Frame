#include "frame/json/parse_level.h"

#include "frame/json/parse_material.h"
#include "frame/json/parse_program.h"
#include "frame/json/parse_scene_tree.h"
#include "frame/json/parse_texture.h"
#include "frame/level.h"
#include "frame/opengl/material.h"
#include "frame/opengl/static_mesh.h"
#include "frame/opengl/texture.h"
#include "frame/program_interface.h"

namespace frame::proto {

class LevelProto : public frame::Level {
   public:
    virtual ~LevelProto() = default;
    LevelProto(glm::uvec2 size, const std::string content, DeviceInterface* device) : frame::Level{} {
        // TODO(anirul): Do correct the code later.
        assert(device->GetDeviceEnum() == RenderingAPIEnum::OPENGL);
        auto proto_level      = LoadProtoFromJson<proto::Level>(content);
        name_                 = proto_level.name();
        default_texture_name_ = proto_level.default_texture_name();
        if (default_texture_name_.empty())
            throw std::runtime_error("should have a default texture.");

        // Include the default cube and quad.
        auto cube_id = opengl::CreateCubeStaticMesh(this);
        if (cube_id == NullId) throw std::runtime_error("Could not create static cube mesh.");
        cube_id_     = cube_id;
        auto quad_id = opengl::CreateQuadStaticMesh(this);
        if (quad_id == NullId) throw std::runtime_error("Could not create static quad mesh.");
        quad_id_ = quad_id;

        // Load textures from proto.
        for (const auto& proto_texture : proto_level.textures()) {
            std::unique_ptr<TextureInterface> texture = ParseBasicTexture(proto_texture, size);
            EntityId texture_id                       = NullId;
            std::string texture_name                  = proto_texture.name();
            if (!texture) {
                throw std::runtime_error(
                    fmt::format("Could not load texture: {}", proto_texture.file_name()));
            }
            texture->SetName(texture_name);
            texture_id = AddTexture(std::move(texture));
            logger_->info(fmt::format("Add a new texture {}, with id [{}].", proto_texture.name(),
                                      texture_id));
            if (!texture_id) {
                throw std::runtime_error(
                    fmt::format("Coudn't save texture {} to level.", proto_texture.name()));
            }
        }

        // Check the default texture is in.
        if (!name_id_map_.count(default_texture_name_)) {
            throw std::runtime_error("no default texture is loaded: " + default_texture_name_);
        }

        // Load programs from proto.
        for (const auto& proto_program : proto_level.programs()) {
            auto program = ParseProgramOpenGL(proto_program, this);
            if (!program) {
                throw std::runtime_error(fmt::format("invalid program: {}", proto_program.name()));
            }
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

std::unique_ptr<LevelInterface> ParseLevel(glm::uvec2 size, const std::filesystem::path& path,
                                           DeviceInterface* device) {
    std::ifstream ifs(path.string().c_str());
    std::string content(std::istreambuf_iterator<char>(ifs), {});
    return std::make_unique<LevelProto>(size, content, device);
}

std::unique_ptr<frame::LevelInterface> ParseLevel(glm::uvec2 size, const std::string& content,
                                                  DeviceInterface* device) {
    return std::make_unique<LevelProto>(size, content, device);
}

}  // End namespace frame::proto.
