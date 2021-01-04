#pragma once

#include <gmock/gmock.h>
#include "../Frame/ProgramInterface.h"

namespace test {

	class ProgramMock : public frame::ProgramInterface
	{
	public:
		MOCK_METHOD(void, LinkShader, (), (override));
		MOCK_METHOD(void, Use, (), (const, override));
		MOCK_METHOD(
			void, 
			Uniform, 
			(const std::string& name, bool value), 
			(const, override));
		MOCK_METHOD(
			void,
			Uniform,
			(const std::string& name, int value),
			(const, override));
		MOCK_METHOD(
			void,
			Uniform,
			(const std::string& name, float value),
			(const, override));
		MOCK_METHOD(
			void,
			Uniform,
			(const std::string& name, const glm::vec2 vec2),
			(const, override));
		MOCK_METHOD(
			void,
			Uniform,
			(const std::string& name, const glm::vec3 vec3),
			(const, override));
		MOCK_METHOD(
			void,
			Uniform,
			(const std::string& name, const glm::vec4 vec4),
			(const, override));
		MOCK_METHOD(
			void,
			Uniform,
			(const std::string& name, const glm::mat4 mat4),
			(const, override));
	};

} // End namespace test
