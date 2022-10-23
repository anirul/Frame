#include "frame/opengl/device_test.h"

#include "frame/file/file_system.h"
#include "frame/json/parse_level.h"

namespace test {

TEST_F(DeviceTest, CreateDeviceTest) {
    EXPECT_TRUE(window_);
    EXPECT_TRUE(window_->GetUniqueDevice());
}

TEST_F(DeviceTest, StartupDeviceWithCameraTest) {
    EXPECT_TRUE(window_);
    window_->GetUniqueDevice()->Startup(std::move(level_));
    EXPECT_TRUE(window_->GetUniqueDevice());
}

}  // End namespace test.
