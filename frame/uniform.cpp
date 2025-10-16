#include "frame/uniform.h"
#include "frame/json/serialize_uniform.h"

namespace frame
{

Uniform::Uniform(const std::string& name, int value)
{
    data_.set_name(name);
    data_.set_type(proto::Uniform::INT);
    data_.set_uniform_enum(proto::Uniform::INVALID_UNIFORM);
    data_.set_uniform_int(value);
}

Uniform::Uniform(
    const std::string& name, glm::uvec2 size, const std::vector<int>& list)
{
    data_.set_name(name);
    data_.set_type(proto::Uniform::INTS);
    data_.set_uniform_enum(proto::Uniform::INVALID_UNIFORM);
    proto::UniformInts uniform_ints;
    uniform_ints.mutable_size()->CopyFrom(json::SerializeSize(size));
    uniform_ints.mutable_values()->Assign(list.begin(), list.end());
    data_.mutable_uniform_ints()->CopyFrom(uniform_ints);
}

Uniform::Uniform(const std::string& name, float value)
{
    data_.set_name(name);
    data_.set_type(proto::Uniform::FLOAT);
    data_.set_uniform_enum(proto::Uniform::INVALID_UNIFORM);
    data_.set_uniform_float(value);
}

Uniform::Uniform(
    const std::string& name, glm::uvec2 size, const std::vector<float>& list)
{
    data_.set_name(name);
    data_.set_type(proto::Uniform::FLOATS);
    data_.set_uniform_enum(proto::Uniform::INVALID_UNIFORM);
    proto::UniformFloats uniform_floats;
    uniform_floats.mutable_size()->CopyFrom(json::SerializeSize(size));
    uniform_floats.mutable_values()->Assign(list.begin(), list.end());
    data_.mutable_uniform_floats()->CopyFrom(uniform_floats);
}

Uniform::Uniform(const std::string& name, glm::vec2 value)
{
    data_.set_name(name);
    data_.set_type(proto::Uniform::FLOAT_VECTOR2);
    data_.set_uniform_enum(proto::Uniform::INVALID_UNIFORM);
    data_.mutable_uniform_vec2()->CopyFrom(
        json::SerializeUniformVector2(value));
}

Uniform::Uniform(const std::string& name, glm::vec3 value)
{
    data_.set_name(name);
    data_.set_type(proto::Uniform::FLOAT_VECTOR3);
    data_.set_uniform_enum(proto::Uniform::INVALID_UNIFORM);
    data_.mutable_uniform_vec3()->CopyFrom(
        json::SerializeUniformVector3(value));
}

Uniform::Uniform(const std::string& name, glm::vec4 value)
{
    data_.set_name(name);
    data_.set_type(proto::Uniform::FLOAT_VECTOR4);
    data_.set_uniform_enum(proto::Uniform::INVALID_UNIFORM);
    data_.mutable_uniform_vec4()->CopyFrom(
        json::SerializeUniformVector4(value));
}

Uniform::Uniform(const std::string& name, glm::mat4 value)
{
    data_.set_name(name);
    data_.set_type(proto::Uniform::FLOAT_MATRIX4);
    data_.set_uniform_enum(proto::Uniform::INVALID_UNIFORM);
    data_.mutable_uniform_mat4()->CopyFrom(
        json::SerializeUniformMatrix4(value));
}

Uniform::Uniform(const UniformInterface& uniform_interface)
{
    data_ = uniform_interface.ToProto();
}

Uniform::Uniform(
    const std::string& name,
    const proto::Uniform::UniformEnum& proto_uniform_enum)
{
    data_.set_name(name);
    data_.set_type(proto::Uniform::INVALID_TYPE);
    data_.set_uniform_enum(proto_uniform_enum);
}

} // End of namespace frame.
