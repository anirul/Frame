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
		// No camera definition.
		EXPECT_THROW(device_->Startup(), std::exception);
		EXPECT_TRUE(device_);
	}

	TEST_F(DeviceTest, StartupDeviceWithCameraTest)
	{
		EXPECT_FALSE(device_);
		EXPECT_TRUE(window_);
		device_ = window_->GetUniqueDevice();
		sgl::SceneTree scene_tree{};
		scene_tree.SetDefaultCamera("camera");
		auto scene_camera = std::make_shared<sgl::SceneCamera>();
		scene_camera->SetName("camera");
		scene_tree.AddNode(scene_camera);
		device_->SetSceneTree(scene_tree);
		EXPECT_NO_THROW(device_->Startup());
		EXPECT_TRUE(device_);
	}

} // End namespace test.
