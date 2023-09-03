#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "frame/json/proto.h"
#include "frame/program_interface.h"
#include "frame/uniform_interface.h"

namespace frame::proto {

	/**
	 * @file From proto to glm.
	 */

	 /**
	  * @brief First start with simple type, this is the template part (int, float,
	  *        short,...).
	  * @param uniform_val: Value to be transfered to a C++ value.
	  */
	template <typename T>
	T ParseUniform(const T& uniform_val) {
		return uniform_val;
	}
	/**
	 * @brief Specialization into glm types (vec2).
	 * @param uniform_vec2: Uniform vector 2 input.
	 * @return Glm vec2 output.
	 */
	glm::vec2 ParseUniform(const UniformVector2& uniform_vec2);
	/**
	 * @brief Specialization into glm types (vec3).
	 * @param uniform_vec3: Uniform vector 3 input.
	 * @return Glm vec3 output.
	 */
	glm::vec3 ParseUniform(const UniformVector3& uniform_vec3);
	/**
	 * @brief Specialization into glm types (vec4).
	 * @param uniform_vec4: Uniform vector 4 input.
	 * @return Glm vec4 output.
	 */
	glm::vec4 ParseUniform(const UniformVector4& uniform_vec4);
	/**
	 * @brief Specialization into glm types (mat4).
	 * @param uniform_mat4: Uniform matrix input.
	 * @return Glm mat4 output.
	 */
	glm::mat4 ParseUniform(const UniformMatrix4& uniform_mat4);
	/**
	 * @brief Specialization into glm types (quat).
	 * @param uniform_quat: Uniform quaternion input.
	 * @return Glm quat output.
	 */
	glm::quat ParseUniform(const UniformQuaternion& uniform_quat);
	/**
	 * @brief Specialization into vector types.
	 * @param name: Name of the uniform.
	 * @param uniform_vec: The vector value.
	 * @param program_interface: Program interface to set the uniform.
	 */
	template <typename T, typename U>
	void ParseUniformVec(
		const std::string& name,
		const U& uniform_vec,
		const ProgramInterface& program_interface)
	{
		std::uint32_t counter = 0;
		for (const T& uniform_val : uniform_vec.values()) {
			program_interface.Uniform(name + '[' + std::to_string(counter++) + ']',
				ParseUniform(uniform_val));
		}
	}
	/**
	 * @brief Register the uniform in the program.
	 * @param uniform: Proto uniform entry.
	 * @param uniform_interface: Reference to a uniform interface.
	 * @param program_interface: Reference to a program interface.
	 */
	void RegisterUniformFromProto(const frame::proto::Uniform& uniform,
		const UniformInterface& uniform_interface,
		const ProgramInterface& program_interface);
	/**
	 * @brief Register the uniform enum in the program.
	 * @param name: Uniform name.
	 * @param uniform_enum: Uniform enum.
	 * @param uniform_interface: Uniform interface.
	 * @param program_interface: Program interface.
	 */
	void RegisterUniformEnumFromProto(
		const std::string& name,
		const frame::proto::Uniform::UniformEnum& uniform_enum,
		const UniformInterface& uniform_interface,
		const ProgramInterface& program_interface);
	/**
	 * @brief Convert frame proto size into pair.
	 * @param size: Proto size param.
	 * @return Size result.
	 */
	std::pair<std::uint32_t, std::uint32_t> ParseSizeInt(
		const frame::proto::Size size);

}  // namespace frame::proto
