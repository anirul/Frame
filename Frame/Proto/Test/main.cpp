#include <gtest/gtest.h>
#include "ParseLevelTest.h"
#include "ParseMaterialTest.h"
#include "ParsePixelTest.h"
#include "ParseProgramTest.h"
#include "ParseSceneTreeTest.h"
#include "ParseTextureTest.h"
#include "ParseUniformTest.h"

int main(int ac, char** av)
{
	testing::InitGoogleTest(&ac, av);
	return RUN_ALL_TESTS();
}