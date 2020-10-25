#include "Convert.h"

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

	void RegisterUniformFromProto(
		const frame::proto::Uniform& uniform, 
		const UniformInterface& uniform_interface, 
		Program& program)
	{
		switch (uniform.value_case())
		{
			case frame::proto::Uniform::kUniformInt:
			{
				program.Uniform(uniform.name(), uniform.uniform_int());
				return;
			}
			case frame::proto::Uniform::kUniformBool:
			{
				program.Uniform(uniform.name(), uniform.uniform_bool());
				return;
			}
			case frame::proto::Uniform::kUniformFloat:
			{
				program.Uniform(uniform.name(), uniform.uniform_float());
				return;
			}
			case frame::proto::Uniform::kUniformEnum:
			{
				RegisterUniformEnumFromProto(
					uniform.uniform_enum(), 
					uniform_interface, 
					program);
				return;
			}
			case frame::proto::Uniform::kUniformVec2:
			{
				program.Uniform(
					uniform.name(),
					ParseUniform(uniform.uniform_vec2()));
				return;
			}
			case frame::proto::Uniform::kUniformVec3:
			{
				program.Uniform(
					uniform.name(),
					ParseUniform(uniform.uniform_vec3()));
				return;
			}
			case frame::proto::Uniform::kUniformVec4:
			{
				program.Uniform(
					uniform.name(),
					ParseUniform(uniform.uniform_vec4()));
				return;
			}
			case frame::proto::Uniform::kUniformMat4:
			{
				program.Uniform(
					uniform.name(),
					ParseUniform(uniform.uniform_mat4()));
				return;
			}
			case frame::proto::Uniform::kUniformInts:
			{
				ParseUniformVec<std::int32_t>(
					uniform.name(),
					uniform.uniform_ints(), 
					program);
				return;
			}
			case frame::proto::Uniform::kUniformBools:
			{
				ParseUniformVec<bool>(
					uniform.name(),
					uniform.uniform_bools(), 
					program);
				return;
			}
			case frame::proto::Uniform::kUniformFloats:
			{
				ParseUniformVec<float>(
					uniform.name(),
					uniform.uniform_floats(), 
					program);
				return;
			}
			case frame::proto::Uniform::kUniformVec2S:
			{
				ParseUniformVec<
					glm::vec2, 
					frame::proto::UniformVector2,
					frame::proto::UniformVector2s>(
						uniform.name(),
						uniform.uniform_vec2s(), 
						program);
				return;
			}
			case frame::proto::Uniform::kUniformVec3S:
			{
				ParseUniformVec<
					glm::vec3, 
					frame::proto::UniformVector3,
					frame::proto::UniformVector3s>(
						uniform.name(),
						uniform.uniform_vec3s(), 
						program);
				return;
			}
			case frame::proto::Uniform::kUniformVec4S:
			{
				ParseUniformVec<
					glm::vec4, 
					frame::proto::UniformVector4,
					frame::proto::UniformVector4s>(
						uniform.name(),
						uniform.uniform_vec4s(), 
						program);
				return;
			}
			case frame::proto::Uniform::kUniformMat4S:
			{
				ParseUniformVec<
					glm::mat4, 
					frame::proto::UniformMatrix4,
					frame::proto::UniformMatrix4s>(
						uniform.name(),
						uniform.uniform_mat4s(), 
						program);
				return;
			}
			default:
				throw std::runtime_error("Unknown case : in uniform parsing!");

		}
	}

	void RegisterUniformEnumFromProto(
		const frame::proto::Uniform::UniformEnum& uniform_enum, 
		const UniformInterface& uniform_interface, 
		Program& program)
	{
		switch (uniform_enum)
		{
			case frame::proto::Uniform::UniformEnum::
			Uniform_UniformEnum_PROJECTION_MAT4:
			{
				program.Uniform(
					"projection",
					uniform_interface.GetProjection());
				break;
			}
			case frame::proto::Uniform::UniformEnum::
			Uniform_UniformEnum_PROJECTION_INV_MAT4:
			{
				program.Uniform(
					"projection_inv",
					glm::inverse(uniform_interface.GetProjection()));
				break;
			}
			case frame::proto::Uniform::UniformEnum::
			Uniform_UniformEnum_VIEW_MAT4:
			{
				program.Uniform(
					"view",
					uniform_interface.GetView());
				break;
			}
			case frame::proto::Uniform::UniformEnum::
			Uniform_UniformEnum_VIEW_INV_MAT4:
			{
				program.Uniform(
					"view_inv",
					glm::inverse(uniform_interface.GetView()));
				break;
			}
			case frame::proto::Uniform::UniformEnum::
			Uniform_UniformEnum_MODEL_MAT4:
			{
				program.Uniform(
					"model",
					uniform_interface.GetModel());
				break;
			}
			case frame::proto::Uniform::UniformEnum::
			Uniform_UniformEnum_MODEL_INV_MAT4:
			{
				program.Uniform(
					"model_inv",
					glm::inverse(uniform_interface.GetModel()));
				break;
			}
			case frame::proto::Uniform::UniformEnum::
			Uniform_UniformEnum_CAMERA_POSITION_VEC3:
			{
				program.Uniform(
					"camera_position",
					uniform_interface.GetCamera().GetPosition());
				break;
			}
			case frame::proto::Uniform::UniformEnum::
			Uniform_UniformEnum_CAMERA_DIRECTION_VEC3:
			{
				// TODO change this for the correct vector
				program.Uniform(
					"camera_position",
					uniform_interface.GetCamera().GetFront());
				break;
			}
			default:
				throw std::runtime_error("Unknown case : in uniform enum!");
		}
	}

} // End namespace sgl.
