#include "frame/json/serialize_uniform.h"
#include <google/protobuf/repeated_field.h>

namespace frame::json
{

proto::Size SerializeSize(glm::uvec2 size)
{
    proto::Size proto_size;
    proto_size.set_x(size.x);
    proto_size.set_y(size.y);
    return proto_size;
}

proto::UniformVector2 SerializeUniformVector2(glm::vec2 vec)
{
    proto::UniformVector2 proto_uniform_vector2;
    proto_uniform_vector2.set_x(vec.x);
    proto_uniform_vector2.set_y(vec.y);
    return proto_uniform_vector2;
}

proto::UniformVector3 SerializeUniformVector3(glm::vec3 vec)
{
    proto::UniformVector3 proto_uniform_vector3;
    proto_uniform_vector3.set_x(vec.x);
    proto_uniform_vector3.set_y(vec.y);
    proto_uniform_vector3.set_z(vec.z);
    return proto_uniform_vector3;
}

proto::UniformVector4 SerializeUniformVector4(glm::vec4 vec)
{
    proto::UniformVector4 proto_uniform_vector4;
    proto_uniform_vector4.set_w(vec.w);
    proto_uniform_vector4.set_x(vec.x);
    proto_uniform_vector4.set_y(vec.y);
    proto_uniform_vector4.set_z(vec.z);
    return proto_uniform_vector4;
}

proto::UniformMatrix4 SerializeUniformMatrix4(glm::mat4 mat)
{
    proto::UniformMatrix4 proto_uniform_matrix4;
    proto_uniform_matrix4.set_m11(mat[0][0]);
    proto_uniform_matrix4.set_m12(mat[0][1]);
    proto_uniform_matrix4.set_m13(mat[0][2]);
    proto_uniform_matrix4.set_m14(mat[0][3]);
    proto_uniform_matrix4.set_m21(mat[1][0]);
    proto_uniform_matrix4.set_m22(mat[1][1]);
    proto_uniform_matrix4.set_m23(mat[1][2]);
    proto_uniform_matrix4.set_m24(mat[1][3]);
    proto_uniform_matrix4.set_m31(mat[2][0]);
    proto_uniform_matrix4.set_m32(mat[2][1]);
    proto_uniform_matrix4.set_m33(mat[2][2]);
    proto_uniform_matrix4.set_m34(mat[2][3]);
    proto_uniform_matrix4.set_m41(mat[3][0]);
    proto_uniform_matrix4.set_m42(mat[3][1]);
    proto_uniform_matrix4.set_m43(mat[3][2]);
    proto_uniform_matrix4.set_m44(mat[3][3]);
    return proto_uniform_matrix4;
}

proto::Uniform SerializeUniform(
    const UniformInterface& uniform_interface,
    const LevelInterface& level_interface)
{
    proto::Uniform proto_uniform;
    for (const auto material_id : level_interface.GetMaterials())
    {
        auto& material_interface =
            level_interface.GetMaterialFromId(material_id);
        for (auto texture_id : material_interface.GetTextureIds())
        {
            if (material_interface.GetInnerName(texture_id) ==
                uniform_interface.GetName())
            {
                return proto_uniform;
            }
        }
    }
    proto_uniform.set_name(uniform_interface.GetName());
    switch (uniform_interface.GetData().type())
    {
    case proto::Uniform::INVALID_TYPE: {
        switch (uniform_interface.GetData().uniform_enum())
        {
        case proto::Uniform::INVALID_UNIFORM:
            throw std::runtime_error(std::format(
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
            proto_uniform.set_uniform_enum(
                uniform_interface.GetData().uniform_enum());
            break;
        default:
            throw std::runtime_error(std::format(
                "Invalid Enum[{}] for uniform [{}]?",
                proto::Uniform::UniformEnum_Name(
                    uniform_interface.GetData().uniform_enum()),
                uniform_interface.GetName()));
        }
        break;
    }
    case proto::Uniform::INT:
        proto_uniform.set_uniform_int(
            uniform_interface.GetData().uniform_int());
        break;
    case proto::Uniform::INTS: {
        proto::UniformInts proto_uniform_ints;
        std::copy(
            uniform_interface.GetData().uniform_ints().values().begin(),
            uniform_interface.GetData().uniform_ints().values().end(),
            google::protobuf::RepeatedFieldBackInserter(
                proto_uniform_ints.mutable_values()));
        proto::Size proto_size;
        proto_size.set_x(uniform_interface.GetData().uniform_ints().size().x());
        proto_size.set_y(uniform_interface.GetData().uniform_ints().size().y());
        proto_uniform_ints.mutable_size()->CopyFrom(proto_size);
        proto_uniform.mutable_uniform_ints()->CopyFrom(proto_uniform_ints);
        break;
    }
    case proto::Uniform::FLOAT:
        proto_uniform.set_uniform_float(
            uniform_interface.GetData().uniform_float());
        break;
    case proto::Uniform::FLOATS: {
        proto::UniformFloats proto_uniform_floats;
        std::copy(
            uniform_interface.GetData().uniform_floats().values().begin(),
            uniform_interface.GetData().uniform_floats().values().end(),
            google::protobuf::RepeatedFieldBackInserter(
                proto_uniform_floats.mutable_values()));
        proto::Size proto_size;
        proto_size.set_x(
            uniform_interface.GetData().uniform_floats().size().x());
        proto_size.set_y(
            uniform_interface.GetData().uniform_floats().size().y());
        proto_uniform_floats.mutable_size()->CopyFrom(proto_size);
        proto_uniform.mutable_uniform_floats()->CopyFrom(proto_uniform_floats);
        break;
    }
    case proto::Uniform::FLOAT_VECTOR2:
        proto_uniform.mutable_uniform_vec2()->CopyFrom(
            uniform_interface.GetData().uniform_vec2());
        break;
    case proto::Uniform::FLOAT_VECTOR3:
        proto_uniform.mutable_uniform_vec3()->CopyFrom(
            uniform_interface.GetData().uniform_vec3());
        break;
    case proto::Uniform::FLOAT_VECTOR4:
        proto_uniform.mutable_uniform_vec4()->CopyFrom(
            uniform_interface.GetData().uniform_vec3());
        break;
    case proto::Uniform::FLOAT_MATRIX4:
        proto_uniform.mutable_uniform_mat4()->CopyFrom(
            uniform_interface.GetData().uniform_mat4());
        break;
    default:
        break;
    }
    return proto_uniform;
}

} // End namespace frame::json.
