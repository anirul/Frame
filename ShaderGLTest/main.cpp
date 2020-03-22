#include <gtest/gtest.h>
#include "BufferTest.h"
#include "CameraTest.h"
#include "DeviceTest.h"
#include "ImageTest.h"
#include "MeshTest.h"
#include "ProgramTest.h"
#include "SceneTest.h"
#include "ShaderTest.h"
#include "TextureTest.h"
#include "VectorTest.h"

int main(int ac, char** av) {
	testing::InitGoogleTest(&ac, av);
	return RUN_ALL_TESTS();
}