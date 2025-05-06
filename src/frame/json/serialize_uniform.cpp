#include "frame/json/serialize_uniform.h"
#include <google/protobuf/repeated_field.h>

namespace frame::proto
{

proto::Uniform SerializeUniform(const UniformInterface& uniform_interface)
{
    proto::Uniform proto_uniform;
    proto_uniform.set_name(uniform_interface.GetName());
    switch (uniform_interface.GetType())
    {
    case proto::Uniform::INVALID_TYPE: {
        switch (uniform_interface.GetUniformEnum())
        {
        case proto::Uniform::INVALID_UNIFORM:
            throw std::runtime_error(
                std::format(
                    "Invalid Type and Enum for uniform [{}]?",
                    uniform_interface.GetName()));
        case proto::Uniform::PROJECTION_MAT4:
            [[fallthrough]];
        case proto::Uniform::PROJECTION_INV_MAT4:
            [[fallthrough]];
        case proto::Uniform::VIEW_MAT4:
            [[fallthrough]];
        case proto::Uniform::VIEW_INV_MAT4:
            [[fallthrough]];
        case proto::Uniform::MODEL_MAT4:
            [[fallthrough]];
        case proto::Uniform::MODEL_INV_MAT4:
            [[fallthrough]];
        case proto::Uniform::FLOAT_TIME_S:
            proto_uniform.set_uniform_enum(uniform_interface.GetUniformEnum());
            break;
        default:
            throw std::runtime_error(
                std::format(
                    "Invalid Enum[{}] for uniform [{}]?",
                    proto::Uniform::UniformEnum_Name(
                        uniform_interface.GetUniformEnum()),
                    uniform_interface.GetName()));
        }
        break;
    }
    case proto::Uniform::INT:
        proto_uniform.set_uniform_int(uniform_interface.GetInt());
        break;
    case proto::Uniform::INTS: {
        proto::UniformInts proto_uniform_ints;
        std::copy(
            uniform_interface.GetInts().begin(),
            uniform_interface.GetInts().end(),
            google::protobuf::RepeatedFieldBackInserter(
                proto_uniform_ints.mutable_values()));
        proto::Size proto_size;
        proto_size.set_x(uniform_interface.GetSize().x);
        proto_size.set_y(uniform_interface.GetSize().y);
        proto_uniform_ints.mutable_size()->CopyFrom(proto_size);
        proto_uniform.mutable_uniform_ints()->CopyFrom(proto_uniform_ints);
        break;
    }
    case proto::Uniform::FLOAT:
        proto_uniform.set_uniform_float(uniform_interface.GetFloat());
        break;
    case proto::Uniform::FLOATS: {
        proto::UniformFloats proto_uniform_floats;
        std::copy(
            uniform_interface.GetFloats().begin(),
            uniform_interface.GetFloats().end(),
            google::protobuf::RepeatedFieldBackInserter(
                proto_uniform_floats.mutable_values()));
        proto::Size proto_size;
        proto_size.set_x(uniform_interface.GetSize().x);
        proto_size.set_y(uniform_interface.GetSize().y);
        proto_uniform_floats.mutable_size()->CopyFrom(proto_size);
        proto_uniform.mutable_uniform_floats()->CopyFrom(proto_uniform_floats);
        break;
    }
    case proto::Uniform::FLOAT_VECTOR2: {
        proto::UniformVector2 proto_uniform_vector2;
        proto_uniform_vector2.set_x(uniform_interface.GetVec2().x);
        proto_uniform_vector2.set_y(uniform_interface.GetVec2().y);
        proto_uniform.mutable_uniform_vec2()->CopyFrom(proto_uniform_vector2);
        break;
    }
    case proto::Uniform::FLOAT_VECTOR3: {
        proto::UniformVector3 proto_uniform_vector3;
        proto_uniform_vector3.set_x(uniform_interface.GetVec3().x);
        proto_uniform_vector3.set_y(uniform_interface.GetVec3().y);
        proto_uniform_vector3.set_z(uniform_interface.GetVec3().z);
        proto_uniform.mutable_uniform_vec3()->CopyFrom(proto_uniform_vector3);
        break;
    }
    case proto::Uniform::FLOAT_VECTOR4: {
        proto::UniformVector4 proto_uniform_vector4;
        proto_uniform_vector4.set_x(uniform_interface.GetVec4().x);
        proto_uniform_vector4.set_y(uniform_interface.GetVec4().y);
        proto_uniform_vector4.set_z(uniform_interface.GetVec4().z);
        proto_uniform_vector4.set_w(uniform_interface.GetVec4().w);
        proto_uniform.mutable_uniform_vec3()->CopyFrom(proto_uniform_vector4);
        break;
    }
    case proto::Uniform::FLOAT_MATRIX4: {
        proto::UniformMatrix4 proto_uniform_matrix4;
        proto_uniform_matrix4.set_m11(uniform_interface.GetMat4()[0][0]);
        proto_uniform_matrix4.set_m12(uniform_interface.GetMat4()[0][1]);
        proto_uniform_matrix4.set_m13(uniform_interface.GetMat4()[0][2]);
        proto_uniform_matrix4.set_m14(uniform_interface.GetMat4()[0][3]);
        proto_uniform_matrix4.set_m21(uniform_interface.GetMat4()[1][0]);
        proto_uniform_matrix4.set_m22(uniform_interface.GetMat4()[1][1]);
        proto_uniform_matrix4.set_m23(uniform_interface.GetMat4()[1][2]);
        proto_uniform_matrix4.set_m24(uniform_interface.GetMat4()[1][3]);
        proto_uniform_matrix4.set_m31(uniform_interface.GetMat4()[2][0]);
        proto_uniform_matrix4.set_m32(uniform_interface.GetMat4()[2][1]);
        proto_uniform_matrix4.set_m33(uniform_interface.GetMat4()[2][2]);
        proto_uniform_matrix4.set_m34(uniform_interface.GetMat4()[2][3]);
        proto_uniform_matrix4.set_m41(uniform_interface.GetMat4()[3][0]);
        proto_uniform_matrix4.set_m42(uniform_interface.GetMat4()[3][1]);
        proto_uniform_matrix4.set_m43(uniform_interface.GetMat4()[3][2]);
        proto_uniform_matrix4.set_m44(uniform_interface.GetMat4()[3][3]);
        proto_uniform.mutable_uniform_mat4()->CopyFrom(proto_uniform_matrix4);
        break;
    }
    default:
        break;
    }
    return proto_uniform;
}

} // namespace frame::proto