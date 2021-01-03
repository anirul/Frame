#pragma once

#include <gtest/gtest.h>
#include "../Frame/Error.h"

namespace test {

	class ErrorTest : public testing::Test
	{
	public:
		ErrorTest() = default;

	protected:
		frame::Error& error_ = frame::Error::GetInstance();
	};

} // End namespace test.
