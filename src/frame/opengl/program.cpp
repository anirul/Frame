#include "program.h"

#include <absl/strings/match.h>
#include <absl/strings/string_view.h>

#include <regex>
#include <stdexcept>
#include <string_view>

#include "frame/logger.h"

namespace frame::opengl {

Program::Program() { program_id_ = glCreateProgram(); }

Program::~Program() { glDeleteProgram(program_id_); }

void Program::AddShader(const Shader& shader) {
    glAttachShader(program_id_, shader.GetId());
    attached_shaders_.push_back(shader.GetId());
}

void Program::LinkShader() {
    glLinkProgram(program_id_);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::string error_str = fmt::format("Failed to link program [{}], ",
                                            reinterpret_cast<const char*>(gluErrorString(error)));
        GLint program_status  = 0;
        glGetProgramiv(program_id_, GL_LINK_STATUS, &program_status);
        if (program_status != GL_TRUE) {
            error_str += "and GL_LINK_STATUS is GL_FALSE!";
            throw std::runtime_error(error_str);
        }
        error_str += "but GL_LINK_STATUS is GL_TRUE?";
        logger_->warn(error_str);
    }
    for (const auto& id : attached_shaders_) {
        glDetachShader(program_id_, id);
    }
    CreateUniformList();
}

void Program::Use() const { glUseProgram(program_id_); }

void Program::Use(const UniformInterface& uniform_interface) const {
    glUseProgram(program_id_);
    if (HasUniform("projection")) {
        Uniform("projection", uniform_interface.GetProjection());
    }
    if (HasUniform("view")) {
        Uniform("view", uniform_interface.GetView());
    }
    if (HasUniform("model")) {
        Uniform("model", uniform_interface.GetModel());
    }
    if (HasUniform("time_s")) {
        Uniform("time_s", static_cast<float>(uniform_interface.GetDeltaTime()));
    }
    for (const auto& name : uniform_interface.GetFloatNames()) {
        if (HasUniform(name)) {
            Uniform(name, uniform_interface.GetValueFloat(name),
                    uniform_interface.GetSizeFromFloat(name));
        }
    }
    for (const auto& name : uniform_interface.GetIntNames()) {
        if (HasUniform(name)) {
            Uniform(name, uniform_interface.GetValueInt(name),
                    uniform_interface.GetSizeFromInt(name));
        }
    }
}

void Program::Uniform(const std::string& name, bool value) const {
    glUniform1i(GetMemoizeUniformLocation(name), (int)value);
}

void Program::Uniform(const std::string& name, int value) const {
    glUniform1i(GetMemoizeUniformLocation(name), value);
}

void Program::Uniform(const std::string& name, float value) const {
    glUniform1f(GetMemoizeUniformLocation(name), value);
}

void Program::Uniform(const std::string& name, const glm::vec2 vec2) const {
    glUniform2f(GetMemoizeUniformLocation(name), vec2.x, vec2.y);
}

void Program::Uniform(const std::string& name, const glm::vec3 vec3) const {
    glUniform3f(GetMemoizeUniformLocation(name), vec3.x, vec3.y, vec3.z);
}

void Program::Uniform(const std::string& name, const glm::vec4 vec4) const {
    glUniform4f(GetMemoizeUniformLocation(name), vec4.x, vec4.y, vec4.z, vec4.w);
}

void Program::Uniform(const std::string& name, const glm::mat4 mat) const {
    glUniformMatrix4fv(GetMemoizeUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

void Program::Uniform(const std::string& name, const std::vector<float>& vector,
                      glm::uvec2 size) const {
    if (size.y == 0 && size.x == 0) {
        if (vector.size() == 0) {
            logger_->warn("Entered a uniform [{}] without size.", name);
            return;
        }
        throw std::runtime_error(
            fmt::format("Unknown size doesn't know that size equivalent: {}", vector.size()));
    }
    assert(vector.size() == size.x * size.y);
    if (size.y == 1) {
        if (size.x == 1) {
            glUniform1f(GetMemoizeUniformLocation(name), vector[0]);
            return;
        }
        glUniform1fv(GetMemoizeUniformLocation(name), size.x, vector.data());
        return;
    }
    if (size.y == 2) {
        if (size.x == 1) {
            glUniform2f(GetMemoizeUniformLocation(name), vector[0], vector[1]);
            return;
        }
        if (size.x == 2) {
            glUniformMatrix2fv(GetMemoizeUniformLocation(name), 1, GL_FALSE, vector.data());
            return;
        }
    }
    if (size.y == 3) {
        if (size.x == 1) {
            glUniform3f(GetMemoizeUniformLocation(name), vector[0], vector[1], vector[2]);
            return;
        }
        if (size.x == 3) {
            glUniformMatrix3fv(GetMemoizeUniformLocation(name), 1, GL_FALSE, vector.data());
            return;
        }
    }
    if (size.y == 4) {
        if (size.x == 1) {
            glUniform4f(GetMemoizeUniformLocation(name), vector[0], vector[1], vector[2],
                        vector[3]);
            return;
        }
        if (size.x == 4) {
            glUniformMatrix4fv(GetMemoizeUniformLocation(name), 1, GL_FALSE, vector.data());
            return;
        }
    }
    throw std::runtime_error(fmt::format(
        "Unknown size doesn't know that size equivalent: < {}, {} >.", size.x, size.y));
}

void Program::Uniform(const std::string& name, const std::vector<std::int32_t>& vector,
                      glm::uvec2 size /*= { 0, 0 }*/) const {
    if (size.y == 0 && size.x == 0) {
        if (vector.size() == 0) {
            logger_->warn("Entered a uniform [{}] without size.", name);
            return;
        }
        throw std::runtime_error(
            fmt::format("Unknown size doesn't know that size equivalent: {}", vector.size()));
    }
    assert(vector.size() == size.x * size.y);
    if (size.y == 1) {
        if (size.x == 1) {
            glUniform1i(GetMemoizeUniformLocation(name), vector[0]);
            return;
        }
        glUniform1iv(GetMemoizeUniformLocation(name), size.x,
                     static_cast<const GLint*>(vector.data()));
        return;
    }
    if (size.y == 2) {
        if (size.x == 1) {
            glUniform2i(GetMemoizeUniformLocation(name), vector[0], vector[1]);
            return;
        }
    }
    if (size.y == 3) {
        if (size.x == 1) {
            glUniform3i(GetMemoizeUniformLocation(name), vector[0], vector[1], vector[2]);
            return;
        }
    }
    if (size.y == 4) {
        if (size.x == 1) {
            glUniform4i(GetMemoizeUniformLocation(name), vector[0], vector[1], vector[2],
                        vector[3]);
            return;
        }
    }
    throw std::runtime_error(fmt::format(
        "Unknown size doesn't know that size equivalent: < {}, {} >.", size.x, size.y));
}

bool Program::IsUniformInList(const std::string& name) const {
    const auto vector = GetUniformNameList();
    if (std::none_of(vector.cbegin(), vector.cend(),
                     [name](const std::string& inner) { return absl::StrContains(inner, name); })) {
        return false;
    }
    return true;
}

int Program::GetMemoizeUniformLocation(const std::string& name) const {
    if (!memoize_map_.count(name)) {
#ifdef _DEBUG
        if (!IsUniformInList(name)) {
            throw std::runtime_error(fmt::format("Could not find a uniform [{}].", name));
        }
        logger_->info("GetMemoizeUniformLocation [{}].", name);
#endif  // _DEBUG
        int location = glGetUniformLocation(program_id_, name.c_str());
        if (location == -1) {
            throw std::runtime_error(fmt::format("Could not get a location for uniform {}.", name));
        }
        memoize_map_.insert({ name, location });
    }
    return memoize_map_.at(name);
}

void Program::AddInputTextureId(EntityId id) {
    ThrowIsInTextureIds(id);
    input_texture_ids_.push_back(id);
}

void Program::RemoveInputTextureId(EntityId id) {
    auto it = std::find(input_texture_ids_.begin(), input_texture_ids_.end(), id);
    if (it != input_texture_ids_.end()) {
        input_texture_ids_.erase(it);
    }
}

std::vector<EntityId> Program::GetInputTextureIds() const { return input_texture_ids_; }

void Program::AddOutputTextureId(EntityId id) {
    ThrowIsInTextureIds(id);
    output_texture_ids_.push_back(id);
}

void Program::RemoveOutputTextureId(EntityId id) {
    auto it = std::find(output_texture_ids_.begin(), output_texture_ids_.end(), id);
    if (it != output_texture_ids_.end()) {
        output_texture_ids_.erase(it);
    }
}

std::vector<EntityId> Program::GetOutputTextureIds() const { return output_texture_ids_; }

void Program::ThrowIsInTextureIds(EntityId texture_id) const {
    if (std::count(input_texture_ids_.begin(), input_texture_ids_.end(), texture_id)) {
        throw std::runtime_error("Texture: [" + std::to_string(texture_id) +
                                 " is already in input texture ids.");
    }
    if (std::count(output_texture_ids_.begin(), output_texture_ids_.end(), texture_id)) {
        throw std::runtime_error("Texture: [" + std::to_string(texture_id) +
                                 "] is already in output texture ids.");
    }
}

EntityId Program::GetSceneRoot() const {
    // TODO(anirul): Change me with an assert.
    if (!scene_root_) throw std::runtime_error("Should not be null!");
    return scene_root_;
}

void Program::SetSceneRoot(EntityId scene_root) { scene_root_ = scene_root; }

void Program::UnUse() const { glUseProgram(0); }

void Program::CreateUniformList() {
    uniform_list_.clear();
    GLint count = 0;
    glGetProgramiv(program_id_, GL_ACTIVE_UNIFORMS, &count);
    logger_->info("Uniform {} count: {}", name_, count);
    for (GLuint i = 0; i < static_cast<GLuint>(count); ++i) {
        constexpr GLsizei max_size = 256;
        GLsizei length             = 0;
        GLsizei size               = 0;
        GLenum type                = 0;
        GLchar name[max_size];
        glGetActiveUniform(program_id_, i, max_size, &length, &size, &type, name);
        std::string name_str = std::string(name, name + length);
        logger_->info("Uniform: {}, type {}, size [{}].", name, type, size);
        UniformValue uniform_value = { length, size, type, name_str };
        uniform_list_.push_back(uniform_value);
    }
}

std::vector<std::string> Program::GetUniformNameList() const {
    std::vector<std::string> uniform_name_list;
    for (const auto& uniform : uniform_list_) {
        uniform_name_list.push_back(uniform.name);
    }
    return uniform_name_list;
}

bool Program::HasUniform(const std::string& name) const {
    std::vector<std::string> uniform_list = GetUniformNameList();
    if (!std::count(uniform_list.begin(), uniform_list.end(), name)) {
        // To solve the fact that the uniform could end with a `[0]`.
        return std::count(uniform_list.begin(), uniform_list.end(), name + "[0]");
    }
    return true;
}

std::string Program::GetTemporarySceneRoot() const { return temporary_scene_root_; }

void Program::SetTemporarySceneRoot(const std::string& name) { temporary_scene_root_ = name; }

std::unique_ptr<ProgramInterface> CreateProgram(std::istream& vertex_shader_code,
                                                std::istream& pixel_shader_code) {
    std::string vertex_source(std::istreambuf_iterator<char>(vertex_shader_code), {});
    std::string pixel_source(std::istreambuf_iterator<char>(pixel_shader_code), {});
#ifdef _DEBUG
    auto& logger = Logger::GetInstance();
    logger->info("Creating program");
#endif  // _DEBUG
    auto program = std::make_unique<Program>();
    Shader vertex(ShaderEnum::VERTEX_SHADER);
    Shader fragment(ShaderEnum::FRAGMENT_SHADER);
    if (!vertex.LoadFromSource(vertex_source)) {
        throw std::runtime_error(vertex.GetErrorMessage());
    }
    if (!fragment.LoadFromSource(pixel_source)) {
        throw std::runtime_error(fragment.GetErrorMessage());
    }
    program->AddShader(vertex);
    program->AddShader(fragment);
    program->LinkShader();
#ifdef _DEBUG
    logger->info("with pointer := {}", static_cast<void*>(program.get()));
#endif  // _DEBUG
    return std::move(program);
}

}  // End namespace frame::opengl.
