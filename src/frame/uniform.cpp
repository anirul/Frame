#include "frame/uniform.h"

namespace frame
{

Uniform::Uniform(const std::string& name, int value)
    : name_(name), type_(proto::Uniform::INT),
      uniform_enum_(proto::Uniform::INVALID_UNIFORM), size_(1, 1)
{
    value_int_.push_back(value);
}

Uniform::Uniform(
    const std::string& name, glm::uvec2 size, const std::vector<int>& list)
    : name_(name), type_(proto::Uniform::INTS),
      uniform_enum_(proto::Uniform::INVALID_UNIFORM), size_(size)
{
    value_int_.assign(list.begin(), list.end());
}

Uniform::Uniform(const std::string& name, float value)
    : name_(name), type_(proto::Uniform::FLOAT),
      uniform_enum_(proto::Uniform::INVALID_UNIFORM), size_(1, 1)
{
    value_float_.push_back(value);
}

Uniform::Uniform(
    const std::string& name, glm::uvec2 size, const std::vector<float>& list)
    : name_(name), type_(proto::Uniform::INTS),
      uniform_enum_(proto::Uniform::INVALID_UNIFORM), size_(size)
{
    value_float_.assign(list.begin(), list.end());
}

Uniform::Uniform(const std::string& name, glm::vec2 value)
    : name_(name), type_(proto::Uniform::FLOAT_VECTOR2),
      uniform_enum_(proto::Uniform::INVALID_UNIFORM), size_(1, 1)
{
    value_float_.push_back(value.x);
    value_float_.push_back(value.y);
}

Uniform::Uniform(const std::string& name, glm::vec3 value)
    : name_(name), type_(proto::Uniform::FLOAT_VECTOR3),
      uniform_enum_(proto::Uniform::INVALID_UNIFORM), size_(1, 1)
{
    value_float_.push_back(value.x);
    value_float_.push_back(value.y);
    value_float_.push_back(value.z);
}

Uniform::Uniform(const std::string& name, glm::vec4 value)
    : name_(name), type_(proto::Uniform::FLOAT_VECTOR4),
      uniform_enum_(proto::Uniform::INVALID_UNIFORM), size_(1, 1)
{
    value_float_.push_back(value.x);
    value_float_.push_back(value.y);
    value_float_.push_back(value.z);
    value_float_.push_back(value.w);
}

Uniform::Uniform(const std::string& name, glm::mat4 value)
    : name_(name), type_(proto::Uniform::FLOAT_MATRIX4),
      uniform_enum_(proto::Uniform::INVALID_UNIFORM), size_(1, 1)
{
    value_float_.push_back(value[0][0]);
    value_float_.push_back(value[0][1]);
    value_float_.push_back(value[0][2]);
    value_float_.push_back(value[0][3]);
    value_float_.push_back(value[1][0]);
    value_float_.push_back(value[1][1]);
    value_float_.push_back(value[1][2]);
    value_float_.push_back(value[1][3]);
    value_float_.push_back(value[2][0]);
    value_float_.push_back(value[2][1]);
    value_float_.push_back(value[2][2]);
    value_float_.push_back(value[2][3]);
    value_float_.push_back(value[3][0]);
    value_float_.push_back(value[3][1]);
    value_float_.push_back(value[3][2]);
    value_float_.push_back(value[3][3]);
}

Uniform::Uniform(const UniformInterface& uniform_interface)
{
    name_ = uniform_interface.GetName();
    type_ = uniform_interface.GetType();
    uniform_enum_ = uniform_interface.GetUniformEnum();
    size_ = uniform_interface.GetSize();
    value_int_ = uniform_interface.GetInts();
    value_float_ = uniform_interface.GetFloats();
}

std::string Uniform::GetName() const
{
    return name_;
}

void Uniform::SetName(const std::string& name)
{
    name_ = name;
}

proto::Uniform::TypeEnum Uniform::GetType() const
{
    return type_;
}

void Uniform::SetType(proto::Uniform::TypeEnum value)
{
    type_ = value;
}

proto::Uniform::UniformEnum Uniform::GetUniformEnum() const
{
    return uniform_enum_;
}

void Uniform::SetUniformEnum(proto::Uniform::UniformEnum value)
{
    uniform_enum_ = value;
}

glm::uvec2 Uniform::GetSize() const
{
    return size_;
}

void Uniform::SetSize(glm::uvec2 size)
{
    size_ = size;
}

std::vector<std::int32_t> Uniform::GetInts() const
{
    return value_int_;
}

void Uniform::SetInts(const std::vector<std::int32_t>& value)
{
    value_int_ = value;
}

std::vector<float> Uniform::GetFloats() const
{
    return value_float_;
}

void Uniform::SetFloats(const std::vector<float>& value)
{
    value_float_ = value;
}

float Uniform::GetFloat() const
{
#ifdef _DEBUG
    if (type_ != proto::Uniform::FLOAT)
    {
        throw std::runtime_error("Uniform is not a float");
    }
    if (uniform_enum_ != proto::Uniform::INVALID_UNIFORM)
    {
        throw std::runtime_error("Uniform is not a float");
    }
    if (value_float_.size() != 1)
    {
        throw std::runtime_error("Uniform is not a float");
    }
#endif
    return value_float_[0];
}

glm::vec2 Uniform::GetVec2() const
{
#ifdef _DEBUG
    if (type_ != proto::Uniform::FLOAT_VECTOR2)
    {
        throw std::runtime_error("Uniform is not a vec2");
    }
    if (uniform_enum_ != proto::Uniform::INVALID_UNIFORM)
    {
        throw std::runtime_error("Uniform is not a vec2");
    }
    if (value_float_.size() != 2)
    {
        throw std::runtime_error("Uniform is not a vec2");
    }
#endif
    return glm::vec2(value_float_[0], value_float_[1]);
}

glm::vec3 Uniform::GetVec3() const
{
#ifdef _DEBUG
    if (type_ != proto::Uniform::FLOAT_VECTOR3)
    {
        throw std::runtime_error("Uniform is not a vec3");
    }
    if (uniform_enum_ != proto::Uniform::INVALID_UNIFORM)
    {
        throw std::runtime_error("Uniform is not a vec3");
    }
    if (value_float_.size() != 3)
    {
        throw std::runtime_error("Uniform is not a vec3");
    }
#endif
    return glm::vec3(value_float_[0], value_float_[1], value_float_[2]);
}

glm::vec4 Uniform::GetVec4() const
{
#ifdef _DEBUG
    if (type_ != proto::Uniform::FLOAT_VECTOR4)
    {
        throw std::runtime_error("Uniform is not a vec4");
    }
    if (uniform_enum_ != proto::Uniform::INVALID_UNIFORM)
    {
        throw std::runtime_error("Uniform is not a vec4");
    }
    if (value_float_.size() != 4)
    {
        throw std::runtime_error("Uniform is not a vec4");
    }
#endif
    return glm::vec4(
        value_float_[0], value_float_[1], value_float_[2], value_float_[3]);
}

int Uniform::GetInt() const
{
#ifdef _DEBUG
    if (type_ != proto::Uniform::INT)
    {
        throw std::runtime_error("Uniform is not an int");
    }
    if (uniform_enum_ != proto::Uniform::INVALID_UNIFORM)
    {
        throw std::runtime_error("Uniform is not an int");
    }
    if (value_int_.size() != 1)
    {
        throw std::runtime_error("Uniform is not an int");
    }
#endif
    return value_int_[0];
}

glm::mat4 Uniform::GetMat4() const
{
#ifdef _DEBUG
    if (type_ != proto::Uniform::FLOAT_MATRIX4)
    {
        throw std::runtime_error("Uniform is not a mat4");
    }
    if (uniform_enum_ != proto::Uniform::INVALID_UNIFORM)
    {
        throw std::runtime_error("Uniform is not a mat4");
    }
    if (value_float_.size() != 16)
    {
        throw std::runtime_error("Uniform is not a mat4");
    }
#endif
    return glm::mat4(
        value_float_[0],
        value_float_[1],
        value_float_[2],
        value_float_[3],
        value_float_[4],
        value_float_[5],
        value_float_[6],
        value_float_[7],
        value_float_[8],
        value_float_[9],
        value_float_[10],
        value_float_[11],
        value_float_[12],
        value_float_[13],
        value_float_[14],
        value_float_[15]);
}

} // End of namespace frame.
