#include "frame/json/parse_uniform.h"

#include "frame/logger.h"
#include "frame/uniform.h"

namespace frame::proto
{

glm::vec2 ParseUniform(const UniformVector2& uniform_vec2)
{
    return {uniform_vec2.x(), uniform_vec2.y()};
}

glm::vec3 ParseUniform(const UniformVector3& uniform_vec3)
{
    return {uniform_vec3.x(), uniform_vec3.y(), uniform_vec3.z()};
}

glm::vec4 ParseUniform(const UniformVector4& uniform_vec4)
{
    return {
        uniform_vec4.x(), uniform_vec4.y(), uniform_vec4.z(), uniform_vec4.w()};
}

glm::mat4 ParseUniform(const UniformMatrix4& uniform_mat4)
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
    const Uniform& uniform,
    const UniformCollectionInterface& uniform_collection_interface,
    ProgramInterface& program_interface)
{
    switch (uniform.value_oneof_case())
    {
    case Uniform::kUniformInt: {
        std::unique_ptr<frame::UniformInterface> uniform_interface =
            std::make_unique<frame::Uniform>(
                uniform.name(), uniform.uniform_int());
        program_interface.AddUniform(std::move(uniform_interface));
        return;
    }
    case Uniform::kUniformFloat: {
        std::unique_ptr<frame::UniformInterface> uniform_interface =
            std::make_unique<frame::Uniform>(
                uniform.name(), uniform.uniform_float());
        program_interface.AddUniform(std::move(uniform_interface));
        return;
    }
    case Uniform::kUniformEnum: {
        RegisterUniformEnumFromProto(
            uniform.name(),
            uniform.uniform_enum(),
            uniform_collection_interface,
            program_interface);
        return;
    }
    case Uniform::kUniformVec2: {
        std::unique_ptr<frame::UniformInterface> uniform_interface =
            std::make_unique<frame::Uniform>(
                uniform.name(), ParseUniform(uniform.uniform_vec2()));
        program_interface.AddUniform(std::move(uniform_interface));
        return;
    }
    case Uniform::kUniformVec3: {
        std::unique_ptr<frame::UniformInterface> uniform_interface =
            std::make_unique<frame::Uniform>(
                uniform.name(), ParseUniform(uniform.uniform_vec3()));
        program_interface.AddUniform(std::move(uniform_interface));
        return;
    }
    case Uniform::kUniformVec4: {
        std::unique_ptr<frame::UniformInterface> uniform_interface =
            std::make_unique<frame::Uniform>(
                uniform.name(), ParseUniform(uniform.uniform_vec4()));
        program_interface.AddUniform(std::move(uniform_interface));
        return;
    }
    case Uniform::kUniformMat4: {
        std::unique_ptr<frame::UniformInterface> uniform_interface =
            std::make_unique<frame::Uniform>(
                uniform.name(), ParseUniform(uniform.uniform_mat4()));
        program_interface.AddUniform(std::move(uniform_interface));
        return;
    }
    case Uniform::kUniformInts: {
        ParseUniformVec<std::int32_t>(
            uniform.name(), uniform.uniform_ints(), program_interface);
        return;
    }
    case Uniform::kUniformFloats: {
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
    const Uniform::UniformEnum& uniform_enum,
    const UniformCollectionInterface& uniform_collection_interface,
    ProgramInterface& program_interface)
{
    switch (uniform_enum)
    {
    case Uniform::PROJECTION_MAT4: {
        std::unique_ptr<frame::UniformInterface> uniform_interface =
            std::make_unique<frame::Uniform>(
                name,
                uniform_collection_interface.GetUniform("projection")
                    .GetMat4());
        program_interface.AddUniform(std::move(uniform_interface));
        break;
    }
    case Uniform::PROJECTION_INV_MAT4: {
        std::unique_ptr<frame::UniformInterface> uniform_interface =
            std::make_unique<frame::Uniform>(
                name,
                glm::inverse(
                    uniform_collection_interface.GetUniform("projection")
                        .GetMat4()));
        program_interface.AddUniform(std::move(uniform_interface));
        break;
    }
    case Uniform::VIEW_MAT4: {
        std::unique_ptr<frame::UniformInterface> uniform_interface =
            std::make_unique<frame::Uniform>(
                name,
                uniform_collection_interface.GetUniform("view").GetMat4());
        program_interface.AddUniform(std::move(uniform_interface));
        break;
    }
    case Uniform::VIEW_INV_MAT4: {
        std::unique_ptr<frame::UniformInterface> uniform_interface =
            std::make_unique<frame::Uniform>(
                name,
                glm::inverse(
                    uniform_collection_interface.GetUniform("view").GetMat4()));
        program_interface.AddUniform(std::move(uniform_interface));
        break;
    }
    case Uniform::MODEL_MAT4: {
        std::unique_ptr<frame::UniformInterface> uniform_interface =
            std::make_unique<frame::Uniform>(
                name,
                uniform_collection_interface.GetUniform("model").GetMat4());
        program_interface.AddUniform(std::move(uniform_interface));
        break;
    }
    case Uniform::MODEL_INV_MAT4: {
        std::unique_ptr<frame::UniformInterface> uniform_interface =
            std::make_unique<frame::Uniform>(
                name,
                glm::inverse(uniform_collection_interface.GetUniform("model")
                                 .GetMat4()));
        program_interface.AddUniform(std::move(uniform_interface));
        break;
    }
    case Uniform::FLOAT_TIME_S: {
        static Logger& logger_ = Logger::GetInstance();
        logger_->info(
            "set {} := {}",
            name,
            static_cast<float>(
                uniform_collection_interface.GetUniform("time").GetFloat()));
        std::unique_ptr<frame::UniformInterface> uniform_interface =
            std::make_unique<frame::Uniform>(
                name,
                static_cast<float>(
                    uniform_collection_interface.GetUniform("time")
                        .GetFloat()));
        program_interface.AddUniform(std::move(uniform_interface));
        break;
    }
    default:
        throw std::runtime_error("Unknown case : in uniform enum!");
    }
}

std::pair<std::uint32_t, std::uint32_t> ParseSizeInt(const Size size)
{
    return std::make_pair<std::uint32_t, std::uint32_t>(
        static_cast<std::uint32_t>(size.x()),
        static_cast<std::uint32_t>(size.y()));
}

} // namespace frame::proto
