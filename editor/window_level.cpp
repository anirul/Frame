#include "window_level.h"

#include <cstdint>
#include <fstream>
#include <filesystem>
#include <unordered_set>
#include <cctype>
#include <format>
#include <stdexcept>
#include <string_view>
#include <optional>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <stb_image.h>

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"
#include "frame/json/parse_pixel.h"
#include "frame/json/program_catalog.h"
#include "frame/json/serialize_json.h"
#include "frame/json/serialize_level.h"
#include "frame/logger.h"
#include "frame/vulkan/device.h"
#include "frame/gui/window_file_dialog.h"
#include "frame/gui/window_message_box.h"

namespace frame::gui
{

namespace
{

bool HasNodeName(const frame::proto::SceneTree& scene_tree, const std::string& name)
{
    if (name.empty())
    {
        return false;
    }
    for (const auto& node : scene_tree.node_matrices())
    {
        if (node.name() == name)
        {
            return true;
        }
    }
    for (const auto& node : scene_tree.node_meshes())
    {
        if (node.name() == name)
        {
            return true;
        }
    }
    for (const auto& node : scene_tree.node_cameras())
    {
        if (node.name() == name)
        {
            return true;
        }
    }
    for (const auto& node : scene_tree.node_lights())
    {
        if (node.name() == name)
        {
            return true;
        }
    }
    return false;
}

std::unordered_set<std::string> CollectNodeNames(
    const frame::proto::SceneTree& scene_tree)
{
    std::unordered_set<std::string> names = {};
    for (const auto& node : scene_tree.node_matrices())
    {
        names.insert(node.name());
    }
    for (const auto& node : scene_tree.node_meshes())
    {
        names.insert(node.name());
    }
    for (const auto& node : scene_tree.node_cameras())
    {
        names.insert(node.name());
    }
    for (const auto& node : scene_tree.node_lights())
    {
        names.insert(node.name());
    }
    return names;
}

std::string MakeUniqueNodeName(
    const frame::proto::SceneTree& scene_tree, const std::string& base_name)
{
    auto names = CollectNodeNames(scene_tree);
    const std::string root_name = base_name.empty() ? "Mesh" : base_name;
    std::string candidate = root_name;
    int suffix = 1;
    while (names.contains(candidate))
    {
        candidate = std::format("{}_{}", root_name, suffix++);
    }
    return candidate;
}

void EnsureNodeMatrix(
    frame::proto::SceneTree& scene_tree,
    const std::string& name,
    const std::string& parent)
{
    if (HasNodeName(scene_tree, name))
    {
        return;
    }
    auto* node_matrix = scene_tree.add_node_matrices();
    node_matrix->set_name(name);
    if (!parent.empty())
    {
        node_matrix->set_parent(parent);
    }
    auto* matrix = node_matrix->mutable_matrix();
    matrix->set_m11(1.0f);
    matrix->set_m22(1.0f);
    matrix->set_m33(1.0f);
    matrix->set_m44(1.0f);
}

void EnsureDefaultSceneTree(frame::proto::Level& proto_level)
{
    auto* scene_tree = proto_level.mutable_scene_tree();
    if (scene_tree->default_root_name().empty())
    {
        scene_tree->set_default_root_name("root");
    }
    const std::string root_name = scene_tree->default_root_name();
    EnsureNodeMatrix(*scene_tree, root_name, "");

    if (scene_tree->default_camera_name().empty())
    {
        scene_tree->set_default_camera_name("camera");
    }
    const std::string camera_name = scene_tree->default_camera_name();
    bool has_camera = false;
    for (const auto& camera : scene_tree->node_cameras())
    {
        if (camera.name() == camera_name)
        {
            has_camera = true;
            break;
        }
    }
    if (!has_camera)
    {
        auto* camera = scene_tree->add_node_cameras();
        camera->set_name(camera_name);
        camera->set_parent(root_name);
        camera->mutable_position()->set_x(0.0f);
        camera->mutable_position()->set_y(0.5f);
        camera->mutable_position()->set_z(-2.2f);
        camera->mutable_target()->set_x(0.0f);
        camera->mutable_target()->set_y(0.0f);
        camera->mutable_target()->set_z(0.0f);
        camera->mutable_up()->set_x(0.0f);
        camera->mutable_up()->set_y(1.0f);
        camera->mutable_up()->set_z(0.0f);
        camera->set_fov_degrees(65.0f);
        camera->set_aspect_ratio(1.6f);
        camera->set_near_clip(0.01f);
        camera->set_far_clip(1000.0f);
    }
}

std::string ResolveImportedGltfPath(const std::filesystem::path& file)
{
    std::filesystem::path normalized = file.lexically_normal();
    std::string extension = normalized.extension().string();
    for (auto& c : extension)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (extension != ".gltf" && extension != ".glb")
    {
        throw std::runtime_error("Only .gltf and .glb files are supported.");
    }

    const std::string generic = normalized.generic_string();
    constexpr std::string_view kAssetModelPrefix = "asset/model/";
    if (generic.rfind(kAssetModelPrefix, 0) == 0)
    {
        return generic.substr(kAssetModelPrefix.size());
    }
    constexpr std::string_view kModelPrefix = "model/";
    if (generic.rfind(kModelPrefix, 0) == 0)
    {
        return generic.substr(kModelPrefix.size());
    }

    if (normalized.is_absolute())
    {
        const auto model_root = frame::file::FindDirectory("asset/model");
        std::error_code error_code;
        std::filesystem::path relative =
            std::filesystem::relative(normalized, model_root, error_code);
        const std::string relative_generic = relative.generic_string();
        if (!error_code &&
            !relative_generic.empty() &&
            !relative_generic.starts_with(".."))
        {
            return relative_generic;
        }
        return normalized.generic_string();
    }
    return normalized.generic_string();
}

bool IsRaytracingProgram(
    const LevelInterface& level, const frame::EntityId program_id)
{
    if (!program_id)
    {
        return false;
    }
    const auto& program = level.GetProgramFromId(program_id);
    const auto key = frame::json::ResolveProgramKey(program.GetData());
    return frame::json::IsRaytracingProgramKey(key);
}

frame::proto::NodeMesh::RenderTimeEnum SelectImportedMeshRenderTime(
    const LevelInterface& level)
{
    const auto scene_program_id =
        level.GetRenderPassProgramId(frame::proto::NodeMesh::SCENE_RENDER_TIME);
    const auto pre_program_id =
        level.GetRenderPassProgramId(frame::proto::NodeMesh::PRE_RENDER_TIME);

    // Prefer scene pass so imported meshes render every frame.
    if (scene_program_id)
    {
        return frame::proto::NodeMesh::SCENE_RENDER_TIME;
    }
    if (pre_program_id)
    {
        return frame::proto::NodeMesh::PRE_RENDER_TIME;
    }
    return frame::proto::NodeMesh::SCENE_RENDER_TIME;
}

frame::proto::NodeMesh::AccelerationStructureEnum SelectAccelerationStructure(
    const LevelInterface& level,
    frame::proto::NodeMesh::RenderTimeEnum render_time_enum)
{
    auto program_id = level.GetRenderPassProgramId(render_time_enum);
    if (!program_id && render_time_enum == frame::proto::NodeMesh::PRE_RENDER_TIME)
    {
        program_id =
            level.GetRenderPassProgramId(frame::proto::NodeMesh::SCENE_RENDER_TIME);
    }
    if (IsRaytracingProgram(level, program_id))
    {
        return frame::proto::NodeMesh::BVH_ACCELERATION;
    }
    return frame::proto::NodeMesh::NO_ACCELERATION;
}

std::string ResolveSerializedFilePath(const std::filesystem::path& file)
{
    auto normalized = file.lexically_normal();
    if (!normalized.is_absolute())
    {
        return normalized.generic_string();
    }
    const auto asset_root = frame::file::FindDirectory("asset");
    std::error_code ec;
    const auto relative = std::filesystem::relative(normalized, asset_root, ec);
    const std::string relative_generic = relative.generic_string();
    if (!ec && !relative_generic.empty() && !relative_generic.starts_with(".."))
    {
        return std::format("asset/{}", relative_generic);
    }
    return normalized.generic_string();
}

void ValidateSkyboxSourceFile(const std::filesystem::path& selected_path)
{
    const auto absolute = std::filesystem::absolute(selected_path).lexically_normal();
    if (!std::filesystem::exists(absolute))
    {
        throw std::runtime_error(
            std::format("Skybox file does not exist: {}", absolute.string()));
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    if (stbi_info(absolute.string().c_str(), &width, &height, &channels) == 0)
    {
        return;
    }

    if (width <= 0 || height <= 0)
    {
        throw std::runtime_error(std::format(
            "Invalid skybox image size: {}x{}.", width, height));
    }

    constexpr std::uint64_t kMaxSkyboxPixelCount = 8ull * 1024ull * 4ull * 1024ull;
    const std::uint64_t pixel_count =
        static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height);
    if (pixel_count > kMaxSkyboxPixelCount)
    {
        throw std::runtime_error(std::format(
            "Skybox image too large ({}x{}). Maximum supported import size is about 8Kx4K.",
            width,
            height));
    }
}

frame::proto::Texture* FindTextureByName(
    frame::proto::Level& proto_level, const std::string& name)
{
    for (auto& texture : *proto_level.mutable_textures())
    {
        if (texture.name() == name)
        {
            return &texture;
        }
    }
    return nullptr;
}

frame::proto::Texture* EnsureTexture(
    frame::proto::Level& proto_level, const std::string& name)
{
    if (auto* texture = FindTextureByName(proto_level, name))
    {
        return texture;
    }
    auto* texture = proto_level.add_textures();
    texture->set_name(name);
    return texture;
}

frame::proto::Program* FindProgramByName(
    frame::proto::Level& proto_level, const std::string& name)
{
    for (auto& program : *proto_level.mutable_programs())
    {
        if (program.name() == name)
        {
            return &program;
        }
    }
    return nullptr;
}

void ConfigureRenderTargetTexture(frame::proto::Texture& texture)
{
    texture.mutable_size()->set_x(-1);
    texture.mutable_size()->set_y(-1);
    texture.mutable_pixel_element_size()->CopyFrom(
        frame::json::PixelElementSize_BYTE());
    texture.mutable_pixel_structure()->CopyFrom(frame::json::PixelStructure_RGB());
}

void ConfigureFileTexture(
    frame::proto::Texture& texture,
    frame::proto::PixelStructure pixel_structure,
    const std::string& file_name)
{
    texture.mutable_pixel_element_size()->CopyFrom(
        frame::json::PixelElementSize_BYTE());
    frame::proto::PixelStructure structure = {};
    structure.set_value(pixel_structure.value());
    texture.mutable_pixel_structure()->CopyFrom(structure);
    texture.set_file_name(file_name);
}

void EnsureSkyboxProgram(frame::proto::Level& proto_level)
{
    for (const auto& program : proto_level.programs())
    {
        if (frame::json::ResolveProgramKey(program) == "cubemap")
        {
            return;
        }
    }

    auto* program = FindProgramByName(proto_level, "CubeMapProgram");
    if (!program)
    {
        program = proto_level.add_programs();
        program->set_name("CubeMapProgram");
    }
    program->set_pipeline_name("cubemap");
    if (program->input_texture_names_size() == 0)
    {
        program->add_input_texture_names("skybox");
    }
    if (program->output_texture_names_size() == 0)
    {
        program->add_output_texture_names("albedo");
    }
    program->mutable_input_scene_type()->set_value(frame::proto::SceneType::CUBE);

    if (program->bindings_size() == 0)
    {
        auto* binding = program->add_bindings();
        binding->set_name("Skybox");
        binding->set_binding(0);
        binding->set_binding_type(
            frame::proto::ProgramBinding::COMBINED_IMAGE_SAMPLER);
        binding->add_stages(frame::proto::ProgramStage::FRAGMENT);
    }
    if (program->uniforms_size() == 0)
    {
        auto add_uniform = [&](const char* name, frame::proto::Uniform::UniformEnum uniform_enum) {
            auto* uniform = program->add_uniforms();
            uniform->set_name(name);
            uniform->set_uniform_enum(uniform_enum);
        };
        add_uniform("projection", frame::proto::Uniform::PROJECTION_MAT4);
        add_uniform("view", frame::proto::Uniform::VIEW_MAT4);
        add_uniform("model", frame::proto::Uniform::MODEL_MAT4);
    }
}

void EnsureProgramInputTextures(frame::proto::Level& proto_level)
{
    const auto rgb = frame::json::PixelStructure_RGB();
    const auto grey = frame::json::PixelStructure_GREY();
    for (const auto& program : proto_level.programs())
    {
        for (const auto& texture_name : program.input_texture_names())
        {
            if (texture_name.empty())
            {
                continue;
            }
            auto* texture = EnsureTexture(proto_level, texture_name);
            if (texture->has_file_name() || texture->has_file_names() ||
                texture->has_size())
            {
                continue;
            }

            if (texture_name == "albedo_texture")
            {
                ConfigureFileTexture(
                    *texture,
                    rgb,
                    "asset/material/plastic/plastic_albedo.png");
            }
            else if (texture_name == "normal_texture")
            {
                ConfigureFileTexture(
                    *texture,
                    rgb,
                    "asset/material/plastic/plastic_normal.png");
            }
            else if (texture_name == "roughness_texture")
            {
                ConfigureFileTexture(
                    *texture,
                    grey,
                    "asset/material/plastic/plastic_roughness.png");
            }
            else if (texture_name == "metallic_texture")
            {
                ConfigureFileTexture(
                    *texture,
                    grey,
                    "asset/material/plastic/plastic_metallic.png");
            }
            else if (texture_name == "ao_texture")
            {
                ConfigureFileTexture(
                    *texture,
                    grey,
                    "asset/material/plastic/plastic_ao.png");
            }
            else if (texture_name == "skybox" || texture_name == "skybox_env")
            {
                texture->set_cubemap(true);
                texture->mutable_pixel_element_size()->CopyFrom(
                    frame::json::PixelElementSize_BYTE());
                texture->mutable_pixel_structure()->CopyFrom(rgb);
                auto* file_names = texture->mutable_file_names();
                file_names->set_positive_x("asset/cubemap/negative_x.png");
                file_names->set_negative_x("asset/cubemap/positive_x.png");
                file_names->set_positive_y("asset/cubemap/negative_y.png");
                file_names->set_negative_y("asset/cubemap/positive_y.png");
                file_names->set_positive_z("asset/cubemap/negative_z.png");
                file_names->set_negative_z("asset/cubemap/positive_z.png");
            }
            else
            {
                ConfigureRenderTargetTexture(*texture);
            }
        }
    }
}

std::optional<std::string> FindCubemapMaterialName(
    const frame::proto::Level& proto_level)
{
    for (const auto& material : proto_level.materials())
    {
        if (material.name() == "CubeMapMaterial")
        {
            return material.name();
        }
    }
    return std::nullopt;
}

std::string EnsureSkyboxMaterialName(frame::proto::Level& proto_level)
{
    if (auto material_name = FindCubemapMaterialName(proto_level))
    {
        return *material_name;
    }
    auto* material = proto_level.add_materials();
    material->set_name("CubeMapMaterial");
    return material->name();
}

std::optional<std::string> FindSkyboxProgramName(const frame::proto::Level& proto_level)
{
    for (const auto& pass : proto_level.render_pass_programs())
    {
        if (pass.render_time_enum() == frame::proto::NodeMesh::SKYBOX_RENDER_TIME)
        {
            return pass.program_name();
        }
    }
    for (const auto& program : proto_level.programs())
    {
        const auto key = frame::json::ResolveProgramKey(program);
        if (key == "cubemap")
        {
            return program.name();
        }
    }
    return std::nullopt;
}

void EnsureSkyboxRenderPass(frame::proto::Level& proto_level)
{
    EnsureSkyboxProgram(proto_level);
    auto* albedo = EnsureTexture(proto_level, "albedo");
    if (!albedo->has_size() && !albedo->has_file_name() &&
        !albedo->has_file_names())
    {
        ConfigureRenderTargetTexture(*albedo);
    }
    if (proto_level.default_texture_name().empty())
    {
        proto_level.set_default_texture_name("albedo");
    }

    auto program_name = FindSkyboxProgramName(proto_level);
    if (!program_name)
    {
        return;
    }
    for (auto& pass : *proto_level.mutable_render_pass_programs())
    {
        if (pass.render_time_enum() == frame::proto::NodeMesh::SKYBOX_RENDER_TIME)
        {
            pass.set_program_name(*program_name);
            return;
        }
    }
    auto* pass = proto_level.add_render_pass_programs();
    pass->set_render_time_enum(frame::proto::NodeMesh::SKYBOX_RENDER_TIME);
    pass->set_program_name(*program_name);
}

void ConfigureCubemapTexture(
    frame::proto::Texture& texture, const std::string& serialized_path)
{
    texture.set_cubemap(true);
    texture.mutable_pixel_element_size()->CopyFrom(
        frame::json::PixelElementSize_BYTE());
    texture.mutable_pixel_structure()->CopyFrom(frame::json::PixelStructure_RGB());
    texture.set_file_name(serialized_path);
}

void EnsureSkyboxNode(
    frame::proto::Level& proto_level, const std::string& material_name)
{
    auto* scene_tree = proto_level.mutable_scene_tree();
    EnsureDefaultSceneTree(proto_level);
    const std::string root_name = scene_tree->default_root_name();
    EnsureNodeMatrix(*scene_tree, "env_holder", root_name);

    frame::proto::NodeMesh* skybox_node = nullptr;
    for (auto& node_mesh : *scene_tree->mutable_node_meshes())
    {
        if (node_mesh.render_time_enum() ==
                frame::proto::NodeMesh::SKYBOX_RENDER_TIME ||
            node_mesh.name() == "CubeMapMesh")
        {
            skybox_node = &node_mesh;
            break;
        }
    }
    if (!skybox_node)
    {
        const std::string node_name = MakeUniqueNodeName(*scene_tree, "CubeMapMesh");
        skybox_node = scene_tree->add_node_meshes();
        skybox_node->set_name(node_name);
    }
    skybox_node->set_parent("env_holder");
    skybox_node->set_mesh_enum(frame::proto::NodeMesh::CUBE);
    skybox_node->set_material_name(material_name);
    skybox_node->set_render_time_enum(frame::proto::NodeMesh::SKYBOX_RENDER_TIME);
}

} // namespace

WindowLevel::WindowLevel(
    DeviceInterface& device,
    DrawGuiInterface& draw_gui,
    const std::string& file_name)
    : WindowJsonFile(file_name, device), device_(device), draw_gui_(draw_gui),
      tab_textures_(draw_gui, [this]() { UpdateJsonEditor(); }),
      tab_programs_(draw_gui, [this]() { UpdateJsonEditor(); })
{
}

void WindowLevel::UpdateJsonEditor()
{
    auto proto_level = frame::json::SerializeLevel(device_.GetLevel());
    std::string json = frame::json::SaveProtoToJson(proto_level);
    SetEditorText(json);

    if (!GetFileName().empty())
    {
        try
        {
            frame::json::SaveProtoToJsonFile(proto_level, GetFileName());
        }
        catch (const std::exception& e)
        {
            frame::Logger::GetInstance()->error(e.what());
        }
    }
}

void WindowLevel::ApplyJsonContent(const std::string& content)
{
    if (device_.GetDeviceEnum() == frame::RenderingAPIEnum::OPENGL)
    {
        auto level = frame::json::ParseLevel(device_.GetSize(), content);
        device_.Startup(std::move(level));
        return;
    }

    if (device_.GetDeviceEnum() == frame::RenderingAPIEnum::VULKAN)
    {
        auto* vulkan_device = dynamic_cast<frame::vulkan::Device*>(&device_);
        if (!vulkan_device)
        {
            throw std::runtime_error("Vulkan device not available.");
        }
        const auto asset_root = frame::file::FindDirectory("asset");
        const auto level_data =
            frame::json::ParseLevelData(device_.GetSize(), content, asset_root);
        vulkan_device->StartupFromLevelData(level_data);
        return;
    }

    throw std::runtime_error("Unsupported rendering backend in editor.");
}

void WindowLevel::ApplyProtoLevel(const frame::proto::Level& proto_level)
{
    auto& logger = frame::Logger::GetInstance();
    const std::string json = frame::json::SaveProtoToJson(proto_level);
    SetEditorText(json);
    if (!GetFileName().empty())
    {
        frame::json::SaveProtoToJsonFile(proto_level, GetFileName());
    }
    logger->info("Applying level update...");
    logger->flush();
    ApplyJsonContent(json);
    logger->info("Level update applied.");
    logger->flush();
}

void WindowLevel::ShowImportGltfDialog()
{
    draw_gui_.AddModalWindow(std::make_unique<WindowFileDialog>(
        "",
        FileDialogEnum::OPEN,
        [this](const std::string& file_name) {
            try
            {
                ImportGltfSceneFromFile(file_name);
            }
            catch (const std::exception& e)
            {
                frame::Logger::GetInstance()->error(e.what());
                draw_gui_.AddModalWindow(std::make_unique<WindowMessageBox>(
                    "Import glTF failed",
                    e.what()));
            }
        }));
}

void WindowLevel::ShowSkyboxFileDialog()
{
    draw_gui_.AddModalWindow(std::make_unique<WindowFileDialog>(
        "",
        FileDialogEnum::OPEN,
        [this](const std::string& file_name) {
            try
            {
                SetSkyboxFromFile(file_name);
            }
            catch (const std::exception& e)
            {
                frame::Logger::GetInstance()->error(e.what());
                draw_gui_.AddModalWindow(std::make_unique<WindowMessageBox>(
                    "Set Skybox failed",
                    e.what()));
            }
        }));
}

void WindowLevel::ImportGltfSceneFromFile(const std::string& file_name)
{
    if (file_name.empty())
    {
        throw std::runtime_error("No glTF file selected.");
    }
    const auto& level = device_.GetLevel();
    if (level.GetPrograms().empty())
    {
        throw std::runtime_error(
            "Current level has no programs. Open a raytracing starter level "
            "(for example asset/json/skinned_mesh.json) before importing glTF.");
    }
    const std::filesystem::path input_path(file_name);
    const std::string mesh_file_name = ResolveImportedGltfPath(input_path);

    auto proto_level = frame::json::SerializeLevel(level);
    EnsureDefaultSceneTree(proto_level);
    auto* scene_tree = proto_level.mutable_scene_tree();

    const std::string root_name = scene_tree->default_root_name();
    EnsureNodeMatrix(*scene_tree, "mesh_holder", root_name);

    const std::string default_base =
        input_path.stem().string().empty()
            ? "ImportedMesh"
            : std::format("{}Mesh", input_path.stem().string());
    const std::string node_name = MakeUniqueNodeName(*scene_tree, default_base);

    auto* node_mesh = scene_tree->add_node_meshes();
    node_mesh->set_name(node_name);
    node_mesh->set_parent("mesh_holder");
    node_mesh->set_file_name(mesh_file_name);

    const auto render_time = SelectImportedMeshRenderTime(level);
    node_mesh->set_render_time_enum(render_time);
    node_mesh->set_acceleration_structure_enum(
        SelectAccelerationStructure(level, render_time));
    node_mesh->set_play_animation(true);

    ApplyProtoLevel(proto_level);
    frame::Logger::GetInstance()->info(
        "Imported glTF scene node '{}' from '{}'.",
        node_name,
        mesh_file_name);
}

void WindowLevel::SetSkyboxFromFile(const std::string& file_name)
{
    if (file_name.empty())
    {
        throw std::runtime_error("No skybox file selected.");
    }
    const std::filesystem::path selected_path(file_name);
    const std::string extension =
        selected_path.extension().string();
    if (extension.empty())
    {
        throw std::runtime_error("Skybox file must have an extension.");
    }
    ValidateSkyboxSourceFile(selected_path);

    auto proto_level = frame::json::SerializeLevel(device_.GetLevel());
    EnsureDefaultSceneTree(proto_level);
    const std::string serialized_path =
        ResolveSerializedFilePath(std::filesystem::absolute(selected_path));

    auto* skybox_texture = EnsureTexture(proto_level, "skybox");
    auto* skybox_env_texture = EnsureTexture(proto_level, "skybox_env");
    ConfigureCubemapTexture(*skybox_texture, serialized_path);
    ConfigureCubemapTexture(*skybox_env_texture, serialized_path);

    const std::string material_name = EnsureSkyboxMaterialName(proto_level);
    EnsureSkyboxRenderPass(proto_level);
    EnsureProgramInputTextures(proto_level);
    EnsureSkyboxNode(proto_level, material_name);
    ApplyProtoLevel(proto_level);

    frame::Logger::GetInstance()->info(
        "Skybox textures updated from '{}'.", serialized_path);
}

bool WindowLevel::AddSkyboxNode()
{
    auto proto_level = frame::json::SerializeLevel(device_.GetLevel());
    const std::string before_json = frame::json::SaveProtoToJson(proto_level);
    const std::string material_name = EnsureSkyboxMaterialName(proto_level);
    EnsureSkyboxRenderPass(proto_level);
    EnsureProgramInputTextures(proto_level);
    EnsureSkyboxNode(proto_level, material_name);
    const std::string after_json = frame::json::SaveProtoToJson(proto_level);

    if (before_json == after_json)
    {
        frame::Logger::GetInstance()->info(
            "Skybox already configured; no change applied.");
        return false;
    }

    ApplyProtoLevel(proto_level);
    frame::Logger::GetInstance()->info(
        "Skybox node configured with material '{}'.",
        material_name);
    return true;
}

bool WindowLevel::DrawCallback()
{
    ImGui::Text(
        "Project: %s",
        GetFileName().empty() ? "<unsaved>" : GetFileName().c_str());
    ImGui::Text(
        "Backend: %s",
        device_.GetDeviceEnum() == frame::RenderingAPIEnum::VULKAN
            ? "Vulkan"
            : "OpenGL");
    ImGui::Separator();

    if (show_json_)
    {
        ImGui::BeginDisabled();
        ImGui::Button("JSON Editor");
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Node Editor"))
        {
            show_json_ = false;
        }
    }
    else
    {
        if (ImGui::Button("JSON Editor"))
        {
            show_json_ = true;
        }
        ImGui::SameLine();
        ImGui::BeginDisabled();
        ImGui::Button("Node Editor");
        ImGui::EndDisabled();
    }

    ImGui::Separator();
    if (ImGui::Button("Import glTF"))
    {
        ShowImportGltfDialog();
    }
    ImGui::SameLine();
    if (ImGui::Button("Set Skybox"))
    {
        ShowSkyboxFileDialog();
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Skybox Node"))
    {
        try
        {
            if (!AddSkyboxNode())
            {
                draw_gui_.AddModalWindow(std::make_unique<WindowMessageBox>(
                    "Skybox already exists",
                    "Scene already has a skybox node and pass setup."));
            }
        }
        catch (const std::exception& e)
        {
            frame::Logger::GetInstance()->error(e.what());
            draw_gui_.AddModalWindow(std::make_unique<WindowMessageBox>(
                "Skybox setup failed",
                e.what()));
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save##level"))
    {
        if (GetFileName().empty())
        {
            draw_gui_.AddModalWindow(std::make_unique<WindowMessageBox>(
                "No project file",
                "Use File -> New Project or Open Project first."));
        }
        else
        {
            auto& level = device_.GetLevel();
            auto proto_level = frame::json::SerializeLevel(level);
            frame::json::SaveProtoToJsonFile(proto_level, GetFileName());
        }
    }
    ImGui::Separator();

    if (show_json_)
    {
        WindowJsonFile::DrawCallback();
    }
    else
    {
        auto& level_assets = device_.GetLevel();
        ImGui::Begin("Assets", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        tab_textures_.Draw(level_assets);
        tab_programs_.Draw(level_assets);
        tab_materials_.Draw(level_assets);
        ImGui::End();

        auto& level_scene = device_.GetLevel();
        tab_scene_.Draw(level_scene);
    }
    return true;
}

} // namespace frame::gui
