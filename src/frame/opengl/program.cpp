#include "program.h"

#include <format>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string_view>

#include <GL/glew.h>
#include <absl/strings/match.h>
#include <absl/strings/string_view.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "frame/json/parse_uniform.h"
#include "frame/logger.h"
#include "frame/uniform.h"

namespace frame::opengl
{

Program::Program(const std::string& name)
{
    SetName(name);
    program_id_ = glCreateProgram();
}

Program::~Program()
{
    glDeleteProgram(program_id_);
}

void Program::AddShader(const Shader& shader)
{
    glAttachShader(program_id_, shader.GetId());
    attached_shaders_.push_back(shader.GetId());
}

void Program::LinkShader()
{
    glLinkProgram(program_id_);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::string error_str =
            std::format("Failed to link program [{}], ", error);
        GLint program_status = 0;
        glGetProgramiv(program_id_, GL_LINK_STATUS, &program_status);
        if (program_status != GL_TRUE)
        {
            error_str += "and GL_LINK_STATUS is GL_FALSE!";
            throw std::runtime_error(error_str);
        }
        error_str += "but GL_LINK_STATUS is GL_TRUE?";
        logger_->warn(error_str);
    }
    for (const auto& id : attached_shaders_)
    {
        glDetachShader(program_id_, id);
    }
    CreateUniformList();
}

void Program::Use() const
{
    glUseProgram(program_id_);
    is_used_ = true;
}

void Program::Use(
    const UniformCollectionInterface& uniform_collection_interface)
{
    glUseProgram(program_id_);
    is_used_ = true;
    for (const auto& name : uniform_collection_interface.GetUniformNames())
    {
        if (HasUniform(name))
        {
            std::unique_ptr<UniformInterface> uniform_interface =
                std::make_unique<Uniform>(
                    uniform_collection_interface.GetUniform(name));
            AddUniform(std::move(uniform_interface));
        }
    }
}

const UniformInterface& Program::GetUniform(const std::string& name) const
{
    if (!HasUniform(name))
    {
        throw std::runtime_error(
            fmt::format(
                "Uniform [{}] not found in program [{}].", name, name_));
    }
    auto it = uniform_map_.find(name);
    return *(it->second);
}

void Program::AddUniform(std::unique_ptr<UniformInterface>&& uniform_interface)
{
    AddUniformInternal(std::move(uniform_interface), false);
}

void Program::AddUniformInternal(
    std::unique_ptr<UniformInterface>&& uniform_interface, bool bypass_check)
{
    if (!uniform_interface)
    {
        // Unknown types yield a null pointer; skip them silently.
        return;
    }
    std::string name = uniform_interface->GetName();
    if (!bypass_check && !HasUniform(name))
    {
        logger_->warn(
            std::format(
                "Uniform [{}] not active in program [{}], skipping.",
                name,
                name_));
        return;
    }
    auto it = uniform_map_.find(name);
    if (it != uniform_map_.end())
    {
        uniform_map_[name] = std::move(uniform_interface);
    }
    else
    {
        uniform_map_.emplace(name, std::move(uniform_interface));
    }
    auto* uniform_ptr = uniform_map_[name].get();
    switch (uniform_ptr->GetData().type())
    {
    case proto::Uniform::INVALID_TYPE:
        break;
    case proto::Uniform::INT:
        glUniform1i(
            GetMemoizeUniformLocation(name),
            uniform_ptr->GetData().uniform_int());
        break;
    case proto::Uniform::INTS: {
        auto& uniform_ints = uniform_ptr->GetData().uniform_ints();
        glUniform1iv(
            GetMemoizeUniformLocation(name),
            static_cast<GLsizei>(
                uniform_ints.size().x() * uniform_ints.size().y()),
            uniform_ints.values().data());
        break;
    }
    case proto::Uniform::FLOAT:
        glUniform1f(
            GetMemoizeUniformLocation(name),
            uniform_ptr->GetData().uniform_float());
        break;
    case proto::Uniform::FLOATS: {
        auto& uniform_floats = uniform_ptr->GetData().uniform_floats();
        glUniform1fv(
            GetMemoizeUniformLocation(name),
            static_cast<GLsizei>(
                uniform_floats.size().x() * uniform_floats.size().y()),
            uniform_floats.values().data());
        break;
    }
    case proto::Uniform::FLOAT_VECTOR2: {
        glm::vec2 value =
            json::ParseUniform(uniform_ptr->GetData().uniform_vec2());
        glUniform2f(GetMemoizeUniformLocation(name), value.x, value.y);
        break;
    }
    case proto::Uniform::FLOAT_VECTOR3: {
        glm::vec3 value =
            json::ParseUniform(uniform_ptr->GetData().uniform_vec3());
        glUniform3f(GetMemoizeUniformLocation(name), value.x, value.y, value.z);
        break;
    }
    case proto::Uniform::FLOAT_VECTOR4: {
        glm::vec4 value =
            json::ParseUniform(uniform_ptr->GetData().uniform_vec4());
        glUniform4f(
            GetMemoizeUniformLocation(name),
            value.x,
            value.y,
            value.z,
            value.w);
        break;
    }
    case proto::Uniform::FLOAT_MATRIX4: {
        glm::mat4 value =
            json::ParseUniform(uniform_ptr->GetData().uniform_mat4());
        glUniformMatrix4fv(
            GetMemoizeUniformLocation(name), 1, GL_FALSE, &value[0][0]);
        break;
    }
    default:
        logger_->error(
            fmt::format(
                "Unknown uniform type [{}] for uniform [{}].",
                static_cast<int>(uniform_ptr->GetData().type()),
                name));
        break;
    }
}

void Program::RemoveUniform(const std::string& name)
{
    auto it = uniform_map_.find(name);
    if (it != uniform_map_.end())
    {
        uniform_map_.erase(it);
    }
}

int Program::GetMemoizeUniformLocation(const std::string& name) const
{
    if (!is_used_)
    {
        throw std::runtime_error(
            "Program is not used, cannot get uniform location.");
    }
    if (!memoize_map_.count(name))
    {
        while (glGetError() != GL_NO_ERROR)
        {
            // Clear the error.
        }
        int location = glGetUniformLocation(program_id_, name.c_str());
        if (location == -1)
        {
            GLenum error = glGetError();
            throw std::runtime_error(
                fmt::format(
                    "Could not get a location for uniform [{}] error: {}.",
                    name,
                    error));
        }
        memoize_map_.insert({name, location});
    }
    return memoize_map_.at(name);
}

void Program::AddInputTextureId(EntityId id)
{
    ThrowIsInTextureIds(id);
    input_texture_ids_.push_back(id);
}

void Program::RemoveInputTextureId(EntityId id)
{
    auto it =
        std::find(input_texture_ids_.begin(), input_texture_ids_.end(), id);
    if (it != input_texture_ids_.end())
    {
        input_texture_ids_.erase(it);
    }
}

std::vector<EntityId> Program::GetInputTextureIds() const
{
    return input_texture_ids_;
}

void Program::AddOutputTextureId(EntityId id)
{
    ThrowIsInTextureIds(id);
    output_texture_ids_.push_back(id);
}

void Program::RemoveOutputTextureId(EntityId id)
{
    auto it =
        std::find(output_texture_ids_.begin(), output_texture_ids_.end(), id);
    if (it != output_texture_ids_.end())
    {
        output_texture_ids_.erase(it);
    }
}

std::vector<EntityId> Program::GetOutputTextureIds() const
{
    return output_texture_ids_;
}

void Program::ThrowIsInTextureIds(EntityId texture_id) const
{
    if (std::count(
            input_texture_ids_.begin(), input_texture_ids_.end(), texture_id))
    {
        throw std::runtime_error(
            "Texture: [" + std::to_string(texture_id) +
            " is already in input texture ids.");
    }
    if (std::count(
            output_texture_ids_.begin(), output_texture_ids_.end(), texture_id))
    {
        throw std::runtime_error(
            "Texture: [" + std::to_string(texture_id) +
            "] is already in output texture ids.");
    }
}

EntityId Program::GetSceneRoot() const
{
    // TODO(anirul): Change me with an assert.
    if (!scene_root_)
    {
        throw std::runtime_error("Scene root should not be null!");
    }
    return scene_root_;
}

void Program::SetSceneRoot(EntityId scene_root)
{
    scene_root_ = scene_root;
}

void Program::UnUse() const
{
    glUseProgram(0);
    is_used_ = false;
}

void Program::CreateUniformList()
{
    uniform_map_.clear();
    GLint count = 0;
    glGetProgramiv(program_id_, GL_ACTIVE_UNIFORMS, &count);
    logger_->info("Uniform [{}] count: {}", name_, count);
    Use();
    for (GLuint i = 0; i < static_cast<GLuint>(count); ++i)
    {
        constexpr GLsizei max_size = 256;
        GLsizei length = 0;
        GLsizei size = 0;
        GLenum type = 0;
        GLchar name[max_size];
        glGetActiveUniform(
            program_id_, i, max_size, &length, &size, &type, name);
        std::string name_str = std::string(name, name + length);
        logger_->info("Uniform: {}, type {}, size [{}].", name, type, size);
        std::unique_ptr<UniformInterface> uniform_interface = nullptr;
        switch (type)
        {
        case GL_FLOAT: {
            float value = 0.0f;
            glGetUniformfv(
                program_id_, GetMemoizeUniformLocation(name_str), &value);
            uniform_interface = std::make_unique<Uniform>(name_str, value);
            break;
        }
        case GL_FLOAT_VEC2: {
            glm::vec2 value;
            glGetUniformfv(
                program_id_,
                GetMemoizeUniformLocation(name_str),
                glm::value_ptr(value));
            uniform_interface = std::make_unique<Uniform>(name_str, value);
            break;
        }
        case GL_FLOAT_VEC3: {
            glm::vec3 value;
            glGetUniformfv(
                program_id_,
                GetMemoizeUniformLocation(name_str),
                glm::value_ptr(value));
            uniform_interface = std::make_unique<Uniform>(name_str, value);
            break;
        }
        case GL_FLOAT_VEC4: {
            glm::vec4 value;
            glGetUniformfv(
                program_id_,
                GetMemoizeUniformLocation(name_str),
                glm::value_ptr(value));
            uniform_interface = std::make_unique<Uniform>(name_str, value);
            break;
        }
        case GL_INT: {
            int value = 0;
            glGetUniformiv(
                program_id_, GetMemoizeUniformLocation(name_str), &value);
            uniform_interface = std::make_unique<Uniform>(name_str, value);
            break;
        }
        case GL_FLOAT_MAT4: {
            glm::mat4 value;
            glGetUniformfv(
                program_id_,
                GetMemoizeUniformLocation(name_str),
                glm::value_ptr(value));
            uniform_interface = std::make_unique<Uniform>(name_str, value);
            break;
        }
        case GL_SAMPLER_CUBE:
            [[fallthrough]];
        case GL_SAMPLER_2D: {
            int value = -1;
            glGetUniformiv(
                program_id_, GetMemoizeUniformLocation(name_str), &value);
            uniform_interface = std::make_unique<Uniform>(name_str, value);
            break;
        }
        default:
            logger_->error(
                std::format("Unknown uniform name: {} type: {}", name, type));
        }
        if (uniform_interface)
        {
            AddUniformInternal(std::move(uniform_interface), true);
        }
    }
    UnUse();
}

std::vector<std::string> Program::GetUniformNameList() const
{
    std::vector<std::string> uniform_name_list;
    for (const auto& [name, uniform] : uniform_map_)
    {
        uniform_name_list.push_back(name);
    }
    return uniform_name_list;
}

bool Program::HasUniform(const std::string& name) const
{
    std::vector<std::string> uniform_list = GetUniformNameList();
    if (!std::count(uniform_list.begin(), uniform_list.end(), name))
    {
        // To solve the fact that the uniform could end with a `[0]`.
        return std::count(
            uniform_list.begin(), uniform_list.end(), name + "[0]");
    }
    return true;
}

std::string Program::GetTemporarySceneRoot() const
{
    return temporary_scene_root_;
}

void Program::SetTemporarySceneRoot(const std::string& name)
{
    temporary_scene_root_ = name;
}

std::unique_ptr<ProgramInterface> CreateProgram(
    const std::string& name,
    const std::string& shader_name,
    std::istream& vertex_shader_code,
    std::istream& pixel_shader_code)
{
    std::string vertex_source(
        std::istreambuf_iterator<char>(vertex_shader_code), {});
    std::string pixel_source(
        std::istreambuf_iterator<char>(pixel_shader_code), {});
#ifdef _DEBUG
    auto& logger = Logger::GetInstance();
    logger->info("Creating program");
#endif // _DEBUG
    auto program = std::make_unique<Program>(name);
    Shader vertex(ShaderEnum::VERTEX_SHADER);
    Shader fragment(ShaderEnum::FRAGMENT_SHADER);
    if (!vertex.LoadFromSource(vertex_source))
    {
        throw std::runtime_error(vertex.GetErrorMessage());
    }
    if (!fragment.LoadFromSource(pixel_source))
    {
        throw std::runtime_error(fragment.GetErrorMessage());
    }
    program->AddShader(vertex);
    program->AddShader(fragment);
    program->LinkShader();
    program->GetData().set_shader(shader_name);
#ifdef _DEBUG
    logger->info("with pointer := {}", static_cast<void*>(program.get()));
#endif // _DEBUG
    return program;
}

std::unique_ptr<ProgramInterface> CreateProgram(
    const proto::Program& proto_program,
    std::istream& vertex_shader_code,
    std::istream& pixel_shader_code)
{
    std::string vertex_source(
        std::istreambuf_iterator<char>(vertex_shader_code), {});
    std::string pixel_source(
        std::istreambuf_iterator<char>(pixel_shader_code), {});
#ifdef _DEBUG
    auto& logger = Logger::GetInstance();
    logger->info("Creating program");
#endif // _DEBUG
    auto program = std::make_unique<Program>(proto_program.name());
    Shader vertex(ShaderEnum::VERTEX_SHADER);
    Shader fragment(ShaderEnum::FRAGMENT_SHADER);
    if (!vertex.LoadFromSource(vertex_source))
    {
        throw std::runtime_error(vertex.GetErrorMessage());
    }
    if (!fragment.LoadFromSource(pixel_source))
    {
        throw std::runtime_error(fragment.GetErrorMessage());
    }
    program->AddShader(vertex);
    program->AddShader(fragment);
    // Need to add the uniform enum list for serialization.
    program->LinkShader();
    // After linking, the program knows about all active uniforms. Preserve the
    // enum identifiers from the proto so serialization keeps the same names.
    for (const auto& uniform : proto_program.uniforms())
    {
        ProgramInterface* program_iface = program.get();
        if (uniform.has_uniform_enum() &&
            program_iface->HasUniform(uniform.name()))
        {
            std::unique_ptr<UniformInterface> uniform_interface =
                std::make_unique<Uniform>(
                    uniform.name(), uniform.uniform_enum());
            // Bypass the check because the uniform exists but may not be
            // currently active due to the INVALID_TYPE placeholder.
            program->AddUniform(std::move(uniform_interface));
        }
    }
    program->GetData().set_shader(proto_program.shader());
#ifdef _DEBUG
    logger->info("with pointer := {}", static_cast<void*>(program.get()));
#endif // _DEBUG
    return program;
}

} // End namespace frame::opengl.
