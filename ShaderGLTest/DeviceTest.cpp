#include "DeviceTest.h"

namespace test {

	TEST_F(DeviceTest, CreateDeviceTest)
	{
		EXPECT_FALSE(device_);
		EXPECT_TRUE(window_);
		device_ = window_->CreateDevice();
		EXPECT_TRUE(device_);
	}

	TEST_F(DeviceTest, StartupDeviceTest)
	{
		EXPECT_FALSE(device_);
		EXPECT_TRUE(window_);
		device_ = window_->CreateDevice();
		device_->Startup(window_->GetSize());
		EXPECT_TRUE(device_);
	}

} // End namespace test.
