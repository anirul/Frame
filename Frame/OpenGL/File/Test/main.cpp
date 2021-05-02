#include <gtest/gtest.h>
#include "LoadProgramTest.h"
#include "LoadStaticMeshTest.h"
#include "LoadTextureTest.h"

int main(int ac, char** av) 
{
	testing::InitGoogleTest(&ac, av);
	return RUN_ALL_TESTS();
}