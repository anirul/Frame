#include "Convert.h"
#include "Logger.h"

namespace sgl {

	glm::vec2 ParseUniform(const frame::proto::UniformVector2& uniform_vec2)
	{
		return glm::vec2(uniform_vec2.x(), uniform_vec2.y());
	}

	glm::vec3 ParseUniform(const frame::proto::UniformVector3& uniform_vec3)
	{
		return glm::vec3(uniform_vec3.x(), uniform_vec3.y(), uniform_vec3.z());
	}

	glm::vec4 ParseUniform(const frame::proto::UniformVector4& uniform_vec4)
	{
		return glm::vec4(
			uniform_vec4.x(),
			uniform_vec4.y(),
			uniform_vec4.z(),
			uniform_vec4.w());
	}

	glm::mat4 ParseUniform(const frame::proto::UniformMatrix4& uniform_mat4)
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

	glm::quat ParseUniform(const frame::proto::UniformQuaternion& uniform_quat)
	{
		return glm::quat(
			uniform_quat.w(),
			uniform_quat.x(),
			uniform_quat.y(),
			uniform_quat.z());
	}

	void RegisterUniformFromProto(
		const frame::proto::Uniform& uniform, 
		const UniformInterface& uniform_interface, 
		const ProgramInterface& program_interface)
	{
		switch (uniform.value_oneof_case())
		{
			case frame::proto::Uniform::kUniformInt:
			{
				program_interface.Uniform(
					uniform.name(), 
					uniform.uniform_int());
				return;
			}
			case frame::proto::Uniform::kUniformBool:
			{
				program_interface.Uniform(
					uniform.name(), 
					uniform.uniform_bool());
				return;
			}
			case frame::proto::Uniform::kUniformFloat:
			{
				program_interface.Uniform(
					uniform.name(), 
					uniform.uniform_float());
				return;
			}
			case frame::proto::Uniform::kUniformEnum:
			{
				RegisterUniformEnumFromProto(
					uniform.name(),
					uniform.uniform_enum(), 
					uniform_interface, 
					program_interface);
				return;
			}
			case frame::proto::Uniform::kUniformVec2:
			{
				program_interface.Uniform(
					uniform.name(),
					ParseUniform(uniform.uniform_vec2()));
				return;
			}
			case frame::proto::Uniform::kUniformVec3:
			{
				program_interface.Uniform(
					uniform.name(),
					ParseUniform(uniform.uniform_vec3()));
				return;
			}
			case frame::proto::Uniform::kUniformVec4:
			{
				program_interface.Uniform(
					uniform.name(),
					ParseUniform(uniform.uniform_vec4()));
				return;
			}
			case frame::proto::Uniform::kUniformMat4:
			{
				program_interface.Uniform(
					uniform.name(),
					ParseUniform(uniform.uniform_mat4()));
				return;
			}
			case frame::proto::Uniform::kUniformInts:
			{
				ParseUniformVec<std::int32_t>(
					uniform.name(),
					uniform.uniform_ints(), 
					program_interface);
				return;
			}
			case frame::proto::Uniform::kUniformBools:
			{
				ParseUniformVec<bool>(
					uniform.name(),
					uniform.uniform_bools(), 
					program_interface);
				return;
			}
			case frame::proto::Uniform::kUniformFloats:
			{
				ParseUniformVec<float>(
					uniform.name(),
					uniform.uniform_floats(), 
					program_interface);
				return;
			}
			case frame::proto::Uniform::kUniformVec2S:
			{
				ParseUniformVec<frame::proto::UniformVector2>(
						uniform.name(),
						uniform.uniform_vec2s(), 
						program_interface);
				return;
			}
			case frame::proto::Uniform::kUniformVec3S:
			{
				ParseUniformVec<frame::proto::UniformVector3>(
						uniform.name(),
						uniform.uniform_vec3s(), 
						program_interface);
				return;
			}
			case frame::proto::Uniform::kUniformVec4S:
			{
				ParseUniformVec<frame::proto::UniformVector4>(
						uniform.name(),
						uniform.uniform_vec4s(), 
						program_interface);
				return;
			}
			case frame::proto::Uniform::kUniformMat4S:
			{
				ParseUniformVec<frame::proto::UniformMatrix4>(
						uniform.name(),
						uniform.uniform_mat4s(), 
						program_interface);
				return;
			}
			default:
				throw std::runtime_error("Unknown case : in uniform parsing!");
		}
	}

	void RegisterUniformEnumFromProto(
		const std::string& name,
		const frame::proto::Uniform::UniformEnum& uniform_enum, 
		const UniformInterface& uniform_interface, 
		const ProgramInterface& program_interface)
	{
		switch (uniform_enum)
		{
			case frame::proto::Uniform::PROJECTION_MAT4:
			{
				program_interface.Uniform(
					name,
					uniform_interface.GetProjection());
				break;
			}
			case frame::proto::Uniform::PROJECTION_INV_MAT4:
			{
				program_interface.Uniform(
					name,
					glm::inverse(uniform_interface.GetProjection()));
				break;
			}
			case frame::proto::Uniform::VIEW_MAT4:
			{
				program_interface.Uniform(
					name,
					uniform_interface.GetView());
				break;
			}
			case frame::proto::Uniform::VIEW_INV_MAT4:
			{
				program_interface.Uniform(
					name,
					glm::inverse(uniform_interface.GetView()));
				break;
			}
			case frame::proto::Uniform::MODEL_MAT4:
			{
				program_interface.Uniform(
					name,
					uniform_interface.GetModel());
				break;
			}
			case frame::proto::Uniform::MODEL_INV_MAT4:
			{
				program_interface.Uniform(
					name,
					glm::inverse(uniform_interface.GetModel()));
				break;
			}
			case frame::proto::Uniform::CAMERA_POSITION_VEC3:
			{
				program_interface.Uniform(
					name,
					uniform_interface.GetCamera().GetPosition());
				break;
			}
			case frame::proto::Uniform::CAMERA_DIRECTION_VEC3:
			{
				const Camera& cam = uniform_interface.GetCamera();
				program_interface.Uniform(
					name,
					cam.GetFront() - cam.GetPosition());
				break;
			}
			case frame::proto::Uniform::FLOAT_TIME_S:
			{
				static Logger& logger_ = Logger::GetInstance();
				logger_->info(
					"set {} := {}", 
					name,
					static_cast<float>(uniform_interface.GetDeltaTime()));
				program_interface.Uniform(
					name,
					static_cast<float>(uniform_interface.GetDeltaTime()));
				break;
			}
			default:
				throw std::runtime_error("Unknown case : in uniform enum!");
		}
	}

} // End namespace sgl.
