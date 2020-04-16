#include <gtest/gtest.h>
#include "BufferTest.h"
#include "CameraTest.h"
#include "DeviceTest.h"
#include "ErrorTest.h"
#include "FrameTest.h"
#include "ImageTest.h"
#include "LightTest.h"
#include "MeshTest.h"
#include "PixelTest.h"
#include "ProgramTest.h"
#include "SceneTest.h"
#include "ShaderTest.h"
#include "TextureTest.h"
#include "WindowTest.h"

int main(int ac, char** av) {
	testing::InitGoogleTest(&ac, av);
	return RUN_ALL_TESTS();
}