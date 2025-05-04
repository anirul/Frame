#include "program.h"

#include <absl/strings/match.h>
#include <absl/strings/string_view.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <format>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string_view>

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
}

void Program::Use(
    const UniformCollectionInterface& uniform_collection_interface)
{
    glUseProgram(program_id_);
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
        throw std::runtime_error(fmt::format(
            "Uniform [{}] not found in program [{}].", name, name_));
    }
    auto it = uniform_map_.find(name);
    return *(it->second);
}

void Program::AddUniform(std::unique_ptr<UniformInterface>&& uniform_interface)
{
    auto it = uniform_map_.find(uniform_interface->GetName());
    std::string name = uniform_interface->GetName();
    if (it != uniform_map_.end())
    {
        uniform_map_.emplace(name, std::move(uniform_interface));
    }
    else
    {
        uniform_map_[name] = std::move(uniform_interface);
    }
    auto* uniform_ptr = uniform_map_[name].get();
    switch (uniform_ptr->GetType())
    {
    case proto::Uniform::INVALID_TYPE:
        break;
    case proto::Uniform::FLOAT:
        glUniform1f(GetMemoizeUniformLocation(name), uniform_ptr->GetFloat());
        break;
    case proto::Uniform::FLOAT_VECTOR2: {
        glm::vec2 value = uniform_ptr->GetVec2();
        glUniform2f(GetMemoizeUniformLocation(name), value.x, value.y);
        break;
    }
    case proto::Uniform::FLOAT_VECTOR3: {
        glm::vec3 value = uniform_ptr->GetVec3();
        glUniform3f(GetMemoizeUniformLocation(name), value.x, value.y, value.z);
        break;
    }
    case proto::Uniform::FLOAT_VECTOR4: {
        glm::vec4 value = uniform_ptr->GetVec4();
        glUniform4f(
            GetMemoizeUniformLocation(name),
            value.x,
            value.y,
            value.z,
            value.w);
        break;
    }
    case proto::Uniform::INT:
        glUniform1i(GetMemoizeUniformLocation(name), uniform_ptr->GetInt());
        break;
    case proto::Uniform::INT_VECTOR2: {
        glm::ivec2 value = uniform_ptr->GetIVec2();
        glUniform2i(GetMemoizeUniformLocation(name), value.x, value.y);
        break;
    }
    case proto::Uniform::INT_VECTOR3: {
        glm::ivec3 value = uniform_ptr->GetIVec3();
        glUniform3i(GetMemoizeUniformLocation(name), value.x, value.y, value.z);
        break;
    }
    case proto::Uniform::INT_VECTOR4: {
        glm::ivec4 value = uniform_ptr->GetIVec4();
        glUniform4i(
            GetMemoizeUniformLocation(name),
            value.x,
            value.y,
            value.z,
            value.w);
        break;
    }
    case proto::Uniform::FLOAT_MATRIX2: {
        glm::mat2 value = uniform_ptr->GetMat2();
        glUniformMatrix2fv(
            GetMemoizeUniformLocation(name), 1, GL_FALSE, &value[0][0]);
        break;
    }
    case proto::Uniform::FLOAT_MATRIX3: {
        glm::mat3 value = uniform_ptr->GetMat3();
        glUniformMatrix3fv(
            GetMemoizeUniformLocation(name), 1, GL_FALSE, &value[0][0]);
        break;
    }
    case proto::Uniform::FLOAT_MATRIX4: {
        glm::mat4 value = uniform_ptr->GetMat4();
        glUniformMatrix4fv(
            GetMemoizeUniformLocation(name), 1, GL_FALSE, &value[0][0]);
        break;
    }
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

bool Program::IsUniformInList(const std::string& name) const
{
    const auto vector = GetUniformNameList();
    if (std::none_of(
            vector.cbegin(), vector.cend(), [name](const std::string& inner) {
                return absl::StrContains(inner, name);
            }))
    {
        return false;
    }
    return true;
}

int Program::GetMemoizeUniformLocation(const std::string& name) const
{
    if (!memoize_map_.count(name))
    {
#ifdef _DEBUG
        if (!IsUniformInList(name))
        {
            throw std::runtime_error(
                fmt::format("Could not find a uniform [{}].", name));
        }
        logger_->info("GetMemoizeUniformLocation [{}].", name);
#endif // _DEBUG
        int location = glGetUniformLocation(program_id_, name.c_str());
        if (location == -1)
        {
            GLenum error = glGetError();
            throw std::runtime_error(fmt::format(
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
        throw std::runtime_error("Should not be null!");
    return scene_root_;
}

void Program::SetSceneRoot(EntityId scene_root)
{
    scene_root_ = scene_root;
}

void Program::UnUse() const
{
    glUseProgram(0);
}

void Program::CreateUniformList() const
{
    uniform_map_.clear();
    GLint count = 0;
    glGetProgramiv(program_id_, GL_ACTIVE_UNIFORMS, &count);
    logger_->info("Uniform [{}] count: {}", name_, count);
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
        case GL_INT_VEC2: {
            glm::ivec2 value;
            glGetUniformiv(
                program_id_,
                GetMemoizeUniformLocation(name_str),
                glm::value_ptr(value));
            uniform_interface = std::make_unique<Uniform>(name_str, value);
            break;
        }
        case GL_INT_VEC3: {
            glm::ivec3 value;
            glGetUniformiv(
                program_id_,
                GetMemoizeUniformLocation(name_str),
                glm::value_ptr(value));
            uniform_interface = std::make_unique<Uniform>(name_str, value);
            break;
        }
        case GL_INT_VEC4: {
            glm::ivec4 value;
            glGetUniformiv(
                program_id_,
                GetMemoizeUniformLocation(name_str),
                glm::value_ptr(value));
            uniform_interface = std::make_unique<Uniform>(name_str, value);
            break;
        }
        case GL_FLOAT_MAT2: {
            glm::mat2 value;
            glGetUniformfv(
                program_id_,
                GetMemoizeUniformLocation(name_str),
                glm::value_ptr(value));
            uniform_interface = std::make_unique<Uniform>(name_str, value);
            break;
        }
        case GL_FLOAT_MAT3: {
            glm::mat3 value;
            glGetUniformfv(
                program_id_,
                GetMemoizeUniformLocation(name_str),
                glm::value_ptr(value));
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
        }
        uniform_map_.emplace(name_str, std::move(uniform_interface));
    }
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
#ifdef _DEBUG
    logger->info("with pointer := {}", static_cast<void*>(program.get()));
#endif // _DEBUG
    return std::move(program);
}

} // End namespace frame::opengl.
