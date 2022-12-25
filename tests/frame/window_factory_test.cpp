#include "frame/window_factory_test.h"

#include "frame/window_factory.h"

namespace test {

TEST_F(WindowFactoryTest, CreateWindowTest) {
    EXPECT_FALSE(window_);
    EXPECT_NO_THROW(window_ = frame::CreateNewWindow(frame::DrawingTargetEnum::NONE));
    EXPECT_TRUE(window_);
}

}  // End namespace test.
