#include "DeviceTest.h"

namespace test {

	TEST_F(DeviceTest, CreateDeviceTest)
	{
		EXPECT_FALSE(device_);
		EXPECT_TRUE(window_);
		device_ = window_->GetUniqueDevice();
		EXPECT_TRUE(device_);
	}

	TEST_F(DeviceTest, StartupDeviceTest)
	{
		EXPECT_FALSE(device_);
		EXPECT_TRUE(window_);
		device_ = window_->GetUniqueDevice();
		device_->Startup();
		EXPECT_TRUE(device_);
	}

} // End namespace test.
