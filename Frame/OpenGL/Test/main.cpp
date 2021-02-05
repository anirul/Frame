#include <gtest/gtest.h>
#include "BufferTest.h"
#include "DeviceTest.h"
#include "FrameBufferTest.h"
#include "LightTest.h"
#include "StaticMeshTest.h"
#include "PixelTest.h"
#include "ProgramTest.h"
#include "ShaderTest.h"
#include "TextureTest.h"

int main(int ac, char** av) 
{
	testing::InitGoogleTest(&ac, av);
	return RUN_ALL_TESTS();
}