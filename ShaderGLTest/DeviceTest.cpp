#include "DeviceTest.h"

namespace test {

	TEST_F(DeviceTest, CreateDeviceTest)
	{
		EXPECT_FALSE(device_);
		device_ = std::make_shared<sgl::Device>(sdl_window_);
		EXPECT_TRUE(device_);
	}

	TEST_F(DeviceTest, StartupDeviceTest)
	{
		EXPECT_FALSE(device_);
		device_ = std::make_shared<sgl::Device>(sdl_window_);
		PostGlewInit();
		auto maybe_error = device_->Startup(std::make_pair<int, int>(640, 480));
		EXPECT_FALSE(maybe_error);
		if (maybe_error)
		{
			std::cerr << maybe_error.value() << std::endl;
		}
		EXPECT_TRUE(device_);
	}

} // End namespace test.
