#pragma once

#include <gtest/gtest.h>
#include "../ShaderGLLib/Error.h"

namespace test {

	class ErrorTest : public testing::Test
	{
	public:
		ErrorTest() = default;

	protected:
		std::shared_ptr<sgl::Error> error_ = sgl::Error::GetInstance();
	};

} // End namespace test.
