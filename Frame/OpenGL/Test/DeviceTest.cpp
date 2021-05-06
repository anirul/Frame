#include "DeviceTest.h"
#include "Frame/File/FileSystem.h"
#include "Frame/Proto/ParseLevel.h"
#include "LevelProtoCreate.h"

namespace test {

	TEST_F(DeviceTest, CreateDeviceTest)
	{
		EXPECT_TRUE(window_);
		EXPECT_TRUE(device_);
	}

	TEST_F(DeviceTest, StartupDeviceWithCameraTest)
	{
		EXPECT_TRUE(window_);
		device_->Startup(level_);
		EXPECT_TRUE(device_);
	}

} // End namespace test.
