#include "frame/json/parse_uniform.h"
#include "frame/json/serialize_uniform.h"
#include "frame/logger.h"
#include "frame/uniform.h"

namespace frame::json
{

glm::uvec2 ParseSize(const frame::proto::Size& size)
{
    return {size.x(), size.y()};
}

glm::vec2 ParseUniform(const proto::UniformVector2& uniform_vec2)
{
    return {uniform_vec2.x(), uniform_vec2.y()};
}

glm::vec3 ParseUniform(const proto::UniformVector3& uniform_vec3)
{
    return {uniform_vec3.x(), uniform_vec3.y(), uniform_vec3.z()};
}

glm::vec4 ParseUniform(const proto::UniformVector4& uniform_vec4)
{
    return {
        uniform_vec4.x(), uniform_vec4.y(), uniform_vec4.z(), uniform_vec4.w()};
}

glm::mat4 ParseUniform(const proto::UniformMatrix4& uniform_mat4)
{
    return glm::mat4(
        uniform_mat4.m11(),
        uniform_mat4.m12(),
        uniform_mat4.m13(),
        uniform_mat4.m14(),
        uniform_mat4.m21(),
        uniform_mat4.m22(),
        uniform_mat4.m23(),
        uniform_mat4.m24(),
        uniform_mat4.m31(),
        uniform_mat4.m32(),
        uniform_mat4.m33(),
        uniform_mat4.m34(),
        uniform_mat4.m41(),
        uniform_mat4.m42(),
        uniform_mat4.m43(),
        uniform_mat4.m44());
}

void RegisterUniformFromProto(
    const proto::Uniform& uniform,
    const UniformCollectionInterface& uniform_collection_interface,
    ProgramInterface& program_interface)
{
    switch (uniform.value_oneof_case())
    {
    case proto::Uniform::kUniformInt: {
        std::unique_ptr<frame::UniformInterface> uniform_interface =
            std::make_unique<frame::Uniform>(
                uniform.name(), uniform.uniform_int());
        program_interface.AddUniform(std::move(uniform_interface));
        return;
    }
    case proto::Uniform::kUniformFloat: {
        std::unique_ptr<frame::UniformInterface> uniform_interface =
            std::make_unique<frame::Uniform>(
                uniform.name(), uniform.uniform_float());
        program_interface.AddUniform(std::move(uniform_interface));
        return;
    }
    case proto::Uniform::kUniformEnum: {
        RegisterUniformEnumFromProto(
            uniform.name(),
            uniform.uniform_enum(),
            program_interface);
        return;
    }
    case proto::Uniform::kUniformVec2: {
        std::unique_ptr<frame::UniformInterface> uniform_interface =
            std::make_unique<frame::Uniform>(
                uniform.name(), ParseUniform(uniform.uniform_vec2()));
        program_interface.AddUniform(std::move(uniform_interface));
        return;
    }
    case proto::Uniform::kUniformVec3: {
        std::unique_ptr<frame::UniformInterface> uniform_interface =
            std::make_unique<frame::Uniform>(
                uniform.name(), ParseUniform(uniform.uniform_vec3()));
        program_interface.AddUniform(std::move(uniform_interface));
        return;
    }
    case proto::Uniform::kUniformVec4: {
        std::unique_ptr<frame::UniformInterface> uniform_interface =
            std::make_unique<frame::Uniform>(
                uniform.name(), ParseUniform(uniform.uniform_vec4()));
        program_interface.AddUniform(std::move(uniform_interface));
        return;
    }
    case proto::Uniform::kUniformMat4: {
        std::unique_ptr<frame::UniformInterface> uniform_interface =
            std::make_unique<frame::Uniform>(
                uniform.name(), ParseUniform(uniform.uniform_mat4()));
        program_interface.AddUniform(std::move(uniform_interface));
        return;
    }
    case proto::Uniform::kUniformInts: {
        ParseUniformVec<std::int32_t>(
            uniform.name(), uniform.uniform_ints(), program_interface);
        return;
    }
    case proto::Uniform::kUniformFloats: {
        ParseUniformVec<float>(
            uniform.name(), uniform.uniform_floats(), program_interface);
        return;
    }
    default:
        throw std::runtime_error("Unknown case : in uniform parsing!");
    }
}

void RegisterUniformEnumFromProto(
    const std::string& name,
    const proto::Uniform::UniformEnum& uniform_enum,
    ProgramInterface& program_interface)
{
    // Register the uniform using its enum so that serialization retains the
    // original identifier. Runtime values will be applied later via the
    // UniformCollectionWrapper.
    switch (uniform_enum)
    {
    case proto::Uniform::PROJECTION_MAT4:
    case proto::Uniform::PROJECTION_INV_MAT4:
    case proto::Uniform::VIEW_MAT4:
    case proto::Uniform::VIEW_INV_MAT4:
    case proto::Uniform::MODEL_MAT4:
    case proto::Uniform::MODEL_INV_MAT4:
    case proto::Uniform::FLOAT_TIME_S: {
        std::unique_ptr<frame::UniformInterface> uniform_interface =
            std::make_unique<frame::Uniform>(name, uniform_enum);
        program_interface.AddUniform(std::move(uniform_interface));
        break;
    }
    default:
        throw std::runtime_error("Unknown case : in uniform enum!");
    }
}

} // namespace frame::json
